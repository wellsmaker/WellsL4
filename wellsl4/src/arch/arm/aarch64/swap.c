
#include <types_def.h>
#include <state/statedata.h>

#include <arch/irq.h>

extern const sword_t _k_neg_eagain;

sword_t arch_swap(word_t key)
{
	_current_thread->arch.swap_return_value = _k_neg_eagain;

	arm64_call_svc();
	irq_unlock(key);

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current_thread->arch.swap_return_value;
}
