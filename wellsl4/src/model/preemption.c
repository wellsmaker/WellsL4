#include <api/errno.h>
#include <arch/irq.h>
#include <state/statedata.h>
#include <default/default.h>
#include <kernel/thread.h>

/*
 * Possibly preempt the _current_thread thread to allow an interrupt to be handled.
 */
exception_t preemption_point(void)
{
    /* Record that we have performed some work. */
    work_units_completed++;

    /*
     * If we have performed a non-trivial amount of work since last time we
     * checked for preemption, and there is an interrupt pending, handle the
     * interrupt.
     *
     * We avoid checking for pending IRQs every call, as our callers tend to
     * call us in a tight loop and checking for pending IRQs can be quite slow.
     */
    if (work_units_completed >= MAX_NUM_WORUNITS_PER_PREEMPTION) 
	{
        work_units_completed = 0;
        if (irq_is_pending()) 
		{
            return EXCEPTION_PREEMPTED;
        } 
#if defined(CONFIG_ENABLE_KERNEL_MCS) 
		else 
		{
            update_timestamp(false);
            if (!check_budget()) 
			{
                return EXCEPTION_PREEMPTED;
            }
        }
#endif
    }

    return EXCEPTION_NONE;
}
