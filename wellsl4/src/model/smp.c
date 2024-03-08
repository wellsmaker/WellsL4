#include <object/tcb.h>
#include <kernel/thread.h>
#include <model/spinlock.h>
#include <state/statedata.h>
#include <model/atomic.h>
#include <arch/cpu.h>
#include <arch/thread.h>

#ifdef CONFIG_SMP
static atomic_t smp_lock;
static atomic_t smp_is_start;

word_t smp_global_lock(void)
{
	word_t key = arch_irq_lock();

	if (!_current_thread->base.smp_lock_count) 
	{
		while (!atomic_cas(&smp_lock, false, true));
	}

	_current_thread->base.smp_lock_count++;

	return key;
}

void smp_global_unlock(word_t key)
{
	if (_current_thread->base.smp_lock_count) 
	{
		_current_thread->base.smp_lock_count--;

		if (!_current_thread->base.smp_lock_count)
		{
			atomic_clear(&smp_lock);
		}
	}

	arch_irq_unlock(key);
}

void smp_retrieve_global_lock(struct ktcb *thread)
{
	if (thread->base.smp_lock_count) 
	{
		arch_irq_lock();
		while (!atomic_cas(&smp_lock, false, true));
	}
}


/* Called from within swap_thread(), so assumes lock already held */
void smp_release_global_lock(struct ktcb *thread)
{
	if (!thread->base.smp_lock_count) 
	{
		atomic_clear(&smp_lock);
	}
}

#if CONFIG_MP_NUM_CPUS > 1
static void smp_init_entry(int key, void *arg)
{
	atomic_t *is_start = arg;

	/* Wait for the signal to begin scheduling */
	while (!atomic_get(is_start));

	/* Switch out of a dummy thread.  Trick cribbed from the main
	 * thread init.  Should probably unify implementations.
	 */
	struct ktcb dummy_thread 
	{
		.base.option = option_essential_option,
		.base.thread_state = state_dummy_state,
	};

	_current_thread = &dummy_thread;
	smp_timer_init();
	swap_thread_unlocked();

	CODE_UNREACHABLE;
}
#endif

void smp_init(void)
{
	(void)atomic_clear(&smp_is_start);

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	arch_start_cpu(1, _interrupt_stack1, CONFIG_ISR_STACK_SIZE,
		smp_init_entry, &smp_is_start);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	arch_start_cpu(2, _interrupt_stack2, CONFIG_ISR_STACK_SIZE,
		smp_init_entry, &smp_is_start);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	arch_start_cpu(3, _interrupt_stack3, CONFIG_ISR_STACK_SIZE,
		smp_init_entry, &smp_is_start);
#endif

	(void)atomic_set(&smp_is_start, true);
}

bool_t smp_cpu_is_vaild(void)
{
	word_t key = arch_irq_lock();
	bool_t is_switch = arch_is_in_isr() || !arch_irq_unlocked(key);

	arch_irq_unlock(key);
	
	return !is_switch;
}

#endif
