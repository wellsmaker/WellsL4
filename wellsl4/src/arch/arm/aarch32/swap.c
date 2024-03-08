#include <types_def.h>
#include <state/statedata.h>
#include <benchmark/timing.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <arch/irq.h>

#ifdef CONFIG_EXECUTION_BENCHMARKING
extern void read_timer_start_of_swap(void);
#endif
extern const sword_t _k_neg_eagain;

/* The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * arch_swap() itself does not do much.
 *
 * It simply stores the intlock key (the BASEPRI value) parameter into
 * _current_thread->basepri, and then triggers a PendSV exception, which does
 * the heavy lifting of context switching.

 * This is the only place we have to save BASEPRI since the other paths to
 * arm_pendsv all come from handling an interrupt, which means we know the
 * interrupts were not locked: in that case the BASEPRI value is 0.
 *
 * Given that arch_swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the thread of the
 * outgoing thread. This is all performed by the hardware, which stores it in
 * its exception stack page_f, created when handling the arm_pendsv exception.
 *
 * On ARMv6-M, the intlock key is represented by the PRIMASK register,
 * as BASEPRI is not available.
 */
sword_t arch_swap(word_t key)
{
#ifdef CONFIG_EXECUTION_BENCHMARKING
	read_timer_start_of_swap();
#endif

	/* store off key and return value */
	_current_thread->arch.basepri = key;
	_current_thread->arch.swap_return_value = _k_neg_eagain;

#if defined(CONFIG_CPU_CORTEX_M)
	/* set pending bit to make sure we will take a PendSV exception */
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

	/* clear mask or enable all irqs to take a pendsv */
	irq_unlock(0);
#elif defined(CONFIG_CPU_CORTEX_R)
	extern void arm_cortex_r_svc(void);
	arm_cortex_r_svc();
	irq_unlock(key);
#endif

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current_thread->arch.swap_return_value;
}
