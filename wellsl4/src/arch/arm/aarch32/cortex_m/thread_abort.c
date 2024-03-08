

/**
 * @file
 * @brief ARM Cortex-M thread_abort() routine
 *
 * The ARM Cortex-M architecture provides its own thread_abort() to deal
 * with different CPU modes (handler vs thread) when a thread aborts. When its
 * entry point returns or when it aborts itself, the CPU is in thread mode and
 * must call swap_thread() (which triggers a service call), but when in handler
 * mode, the CPU must exit handler mode to cause the context switch, and thus
 * must queue the PendSV exception.
 */

#include <kernel/thread.h>
#include <object/tcb.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sys/assert.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <arch/irq.h>
#include <state/statedata.h>


extern void thread_single_abort(struct ktcb *thread);

void thread_abort(struct ktcb *thread)
{
    word_t key;

    key = irq_lock();

    assert_info(!(thread->base.option & option_essential_option),
                "essential thread aborted");

    thread_single_abort(thread);
#if defined(CONFIG_THREAD_MONITOR)
    thread_foreach_exit(thread);
#endif

    if (_current_thread == thread)
    {
        if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == 0)
        {
            (void)swap_thread_irqlock(key);
            CODE_UNREACHABLE;
        }
        else
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }

    /* The abort handler might have altered the ready queue. */
    reschedule_irqlock(key);
}
