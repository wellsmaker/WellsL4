#include <arch/irq.h>
#include <sys/assert.h>

volatile irq_offload_routine_t offload_routine;
static void *offload_param;

/* Called by arm_svc */
void irq_do_offload(void)
{
	offload_routine(offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, void *parameter)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && defined(CONFIG_ASSERT)
	/* ARMv6-M/ARMv8-M Baseline HardFault if you make a SVC call with
	 * interrupts locked.
	 */
	word_t key;

	__asm__ volatile("mrs %0, PRIMASK;" : "=r" (key) : : "memory");
	assert_info(key == 0U, "irq_offload called with interrupts locked\n");
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE && CONFIG_ASSERT */

	sched_lock();
	offload_routine = routine;
	offload_param = parameter;

	__asm__ volatile ("svc %[id]"
			  :
			  : [id] "i" (svc_irq_offload_svc)
			  : "memory");

	offload_routine = NULL;
	sched_unlock();
}
