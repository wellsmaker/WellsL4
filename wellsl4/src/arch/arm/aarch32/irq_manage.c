/**
 * @file
 * @brief ARM Cortex-M and Cortex-R interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */


#include <arch/cpu.h>
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_R)
#include <device.h>
#endif
#include <sys/assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/irq.h>

extern struct isr_table_entry sw_isr_table[];
extern void arm_reserved(void);

#if defined(CONFIG_CPU_CORTEX_M)
#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

word_t arch_is_irq_pending(void)
{
	word_t index;
	
	for (index = 0; index < CONFIG_NUM_IRQS; index ++)
	{
		if (NVIC_GetActive(index) != 0)
			break;
	}
	return index;
}

void arch_irq_enable(word_t irq)
{
	NVIC_EnableIRQ((IRQn_Type)irq);
}

void arch_irq_disable(word_t irq)
{
	NVIC_DisableIRQ((IRQn_Type)irq);
}

sword_t arch_irq_is_enabled(word_t irq)
{
	/* return NVIC->ISER[REG_FROM_IRQ(irq)] & BIT(BIT_FROM_IRQ(irq)); */
	return NVIC_GetEnableIRQ(irq);
}

/**
 * @internal
 *
 * @brief Set an interrupt's sched_prior
 *
 * The sched_prior is verified if ASSERT_ON is enabled. The maximum number
 * of sched_prior levels is a little complex, as there are some hardware
 * sched_prior levels which are reserved.
 *
 * @return N/A
 */
void arm_irq_sched_prior_set(word_t irq, word_t prio, u32_t flag)
{
	/* The kernel may reserve some of the highest sched_prior levels.
	 * So we offset the requested sched_prior level with the number
	 * of sched_prior levels reserved by the kernel.
	 */

#if defined(CONFIG_ZERO_LATENCY_IRQS)
	/* If we have zero latency interrupts, those interrupts will
	 * run at a sched_prior level which is not masked by irq_lock().
	 * Our policy is to express sched_prior levels with special properties
	 * via flag
	 */
	if (flag & IRQ_ZERO_LATENCY)
	{
		prio = _EXC_ZERO_LATENCY_IRQS_PRIO;
	} 
	else 
	{
		prio += _IRQ_PRIO_OFFSET;
	}
#else
	ARG_UNUSED(flag);
	prio += _IRQ_PRIO_OFFSET;
#endif
	/* The last sched_prior level is also used by PendSV exception, but
	 * allow other interrupts to use the same level, even if it ends up
	 * affecting performance (can still be useful on systems with a
	 * reduced set of priorities, like Cortex-M0/M0+).
	 */
	assert_info(prio <= (BIT(DT_NUM_IRQ_PRIO_BITS) - 1),
		 "invalid sched_prior %d! values must be less than %lu\n",
		 prio - _IRQ_PRIO_OFFSET,
		 BIT(DT_NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET));
	NVIC_SetPriority((IRQn_Type)irq, prio);
}

#elif defined(CONFIG_CPU_CORTEX_R)
word_t arch_is_irq_pending(void)
{

}

void arch_irq_enable(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	irq_enable_next_level(dev, (irq >> 8) - 1);
}

void arch_irq_disable(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	irq_disable_next_level(dev, (irq >> 8) - 1);
}

sword_t arch_irq_is_enabled(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	return irq_is_enabled_next_level(dev);
}

/**
 * @internal
 *
 * @brief Set an interrupt's sched_prior
 *
 * The sched_prior is verified if ASSERT_ON is enabled. The maximum number
 * of sched_prior levels is a little complex, as there are some hardware
 * sched_prior levels which are reserved: three for various types of exceptions,
 * and possibly one additional to support zero latency interrupts.
 *
 * @return N/A
 */
void arm_irq_sched_prior_set(word_t irq, word_t prio, u32_t flag)
{
	struct device *dev = sw_isr_table[0].arg;

	if (irq == 0)
		return;

	irq_set_priority_next_level(dev, (irq >> 8) - 1, prio, flag);
}
#endif

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * See arm_reserved().
 *
 * @return N/A
 */
void irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	arm_reserved();
}

#ifdef CONFIG_SYS_POWER_MANAGEMENT
void arch_isr_direct_pm(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_R)
	word_t key;

	/* irq_lock() does what we wan for this CPU */
	key = irq_lock();
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* Lock all interrupts. irq_lock() will on this CPU only disable those
	 * lower than BASEPRI, which is not what we want. See comments in
	 * arch/arm/core/aarch32/isr_wrapper.S
	 */
	__asm__ volatile("cpsid i" : : : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	if (_kernel.idle_ticks) 
	{
		sword_t idle_val = _kernel.idle_ticks;

		_kernel.idle_ticks = 0;
		sys_power_save_idle_exit(idle_val);
	}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_R)
	irq_unlock(key);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile("cpsie i" : : : "memory");
#else
#error Unknown ARM architecture
#endif
}
#endif

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
/**
 *
 * @brief Set the target security state for the given IRQ
 *
 * Function sets the security state (Secure or Non-Secure) targeted
 * by the given irq. It requires ARMv8-M MCU.
 * It is only compiled if ARM_SECURE_FIRMWARE is defined.
 * It should only be called while in Secure state, otherwise, a write attempt
 * to NVIC.ITNS register is write-ignored(WI), as the ITNS register is not
 * banked between security states and, therefore, has no Non-Secure instance.
 *
 * It shall assert if the operation is not performed successfully.
 *
 * @param irq IRQ line
 * @param secure_state 1 if target state is Secure, 0 otherwise.
 *
 * @return N/A
 */
void irq_target_state_set(word_t irq, sword_t secure_state)
{
	if (secure_state)
	{
		/* Set target to Secure */
		if (NVIC_ClearTargetState(irq) != 0)
		{
			user_error("NVIC SetTargetState error");
		}
	} 
	else
	{
		/* Set target state to Non-Secure */
		if (NVIC_SetTargetState(irq) != 1) 
		{
			user_error("NVIC SetTargetState error");
		}
	}
}

/**
 *
 * @brief Determine whether the given IRQ targets the Secure state
 *
 * Function determines whether the given irq targets the Secure state
 * or not (i.e. targets the Non-Secure state). It requires ARMv8-M MCU.
 * It is only compiled if ARM_SECURE_FIRMWARE is defined.
 * It should only be called while in Secure state, otherwise, a read attempt
 * to NVIC.ITNS register is read-as-zero(RAZ), as the ITNS register is not
 * banked between security states and, therefore, has no Non-Secure instance.
 *
 * @param irq IRQ line
 *
 * @return 1 if target state is Secure, 0 otherwise.
 */
sword_t irq_target_state_is_secure(word_t irq)
{
	return NVIC_GetTargetState(irq) == 0;
}

#endif

#ifdef CONFIG_DYNAMIC_INTERRUPTS
sword_t arch_irq_connect_dynamic(word_t irq, word_t sched_prior,
			     void (*routine)(void *parameter), void *parameter,
			     u32_t flag)
{
	isr_install(irq, routine, parameter);
	arm_irq_sched_prior_set(irq, sched_prior, flag);
	return irq;
}

#ifdef CONFIG_DYNAMIC_DIRECT_INTERRUPTS
static FORCE_INLINE void arm_irq_dynamic_direct_isr_dispatch(void)
{
	u32_t irq = __get_IPSR() - 16;

	if (irq < IRQ_TABLE_SIZE) 
	{
		struct isr_table_entry *isr_entry = &sw_isr_table[irq];

		isr_entry->isr(isr_entry->arg);
	}
}

ISR_DIRECT_DECLARE(arm_irq_direct_dynamic_dispatch_reschedule)
{
	arm_irq_dynamic_direct_isr_dispatch();
	return true;
}

ISR_DIRECT_DECLARE(arm_irq_direct_dynamic_dispatch_no_reschedule)
{
	arm_irq_dynamic_direct_isr_dispatch();
	return false;
}

#endif
#endif
