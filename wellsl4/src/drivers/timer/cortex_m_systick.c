#include <drivers/timer/system_timer.h>
#include <sys/util.h>
#include <model/spinlock.h>
#include <kernel/time.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

static spinlock_t time_lock;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)
             
/* Minimum cycles in the future to try to program.  Note that this is
 * NOT simply "enough cycles to get the counter read and reprogrammed
 * reliably" -- it becomes the minimum value of the LOAD register, and
 * thus reflects how much time we can reliably see expire between
 * calls to elapsed() to read the COUNTFLAG bit.  So it needs to be
 * set to be larger than the maximum time the interrupt might be
 * masked.  Choosing a fraction of a tick is probably a good enough
 * default, with an absolute minimum of 1k cyc.
 */

#define MAX_COUNTS        0x00ffffff
#define CLOSING_COUNTS   0xff000000

#define CYC_PER_TICK    (sys_clock_hw_cycles_per_sec()	/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS       ((MAX_COUNTS / CYC_PER_TICK) - 1)
#define MAX_CYCLES      (MAX_TICKS * CYC_PER_TICK)
#define MIN_DELAY       MAX(1024, (CYC_PER_TICK/16))
#define TICKLESS        (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

extern void arm_exc_exit(void);

static u32_t lastload_cycle;

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in clock_isr() and clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = current_cycle + elapsed();
 */
static u32_t current_cycle;

/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 */
static u32_t announced_cycle;

/*
 * This local variable holds the amount of elapsed HW cycles due to
 * SysTick timer wraps ('overflows') and is used in the calculation
 * in elapsed() function, as well as in the updates to current_cycle.
 *
 * Note:
 * Each time current_cycle is updated with the value from overflowed_cycle,
 * the overflowed_cycle must be reset to zero.
 */
static volatile u32_t overflowed_cycle;

/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'current_cycle' may be updated either by the ISR, or when we
 * re-program the SysTick.LOAD register, in clock_set_timeout().
 *
 * Additionally, the function updates the 'overflowed_cycle' counter, that
 * holds the amount of elapsed HW cycles due to (possibly) multiple
 * timer wraps (overflows).
 *
 * Prerequisites:
 * - reprogramming of SysTick.LOAD must be clearing the SysTick.COUNTER
 *   register and the 'overflowed_cycle' counter.
 * - ISR must be clearing the 'overflowed_cycle' counter.
 * - no more than one counter-wrap has occurred between
 *     - the timer reset or the last time the function was called
 *     - and until the current call of the function is completed.
 * - the function is invoked with interrupts disabled.
 */
static u32_t elapsed(void)
{
	u32_t val1 = SysTick->VAL;	/* A */
	u32_t ctrl = SysTick->CTRL;	/* B */
	u32_t val2 = SysTick->VAL;	/* C */

	/* SysTick behavior: The counter wraps at zero automatically,
	 * setting the COUNTFLAG field of the CTRL register when it
	 * does.  Reading the control register automatically clears
	 * that field.
	 *
	 * If the count wrapped...
	 * 1) Before A then COUNTFLAG will be set and val1 >= val2
	 * 2) Between A and B then COUNTFLAG will be set and val1 < val2
	 * 3) Between B and C then COUNTFLAG will be clear and val1 < val2
	 * 4) After C we'll see it next time
	 *
	 * So the count in val2 is post-wrap and lastload_cycle needs to be
	 * added if and only if COUNTFLAG is set or val1 < val2.
	 */
	if ((ctrl & SysTick_CTRL_COUNTFLAG_Msk) || (val1 < val2))
	{
		overflowed_cycle += lastload_cycle;

		/* We know there was a wrap, but we might not have
		 * seen it in CTRL, so clear it. */
		(void)SysTick->CTRL;
	}

	return (lastload_cycle - val2) + overflowed_cycle;
}

/* Callout out of platform assembly, not hooked via IRQ_DYNC_CONNECT... */
void clock_isr(void)
{
	u32_t dticks;

	/* Update overflowed_cycle and clear COUNTFLAG by invoking elapsed() */
	elapsed();

	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	current_cycle += overflowed_cycle;
	overflowed_cycle = 0;

	if (TICKLESS) 
	{
		/* In TICKLESS mode, the SysTick.LOAD is re-programmed
		 * in clock_set_timeout(), followed by resetting of
		 * the counter (VAL = 0).
		 *
		 * If a timer wrap occurs right when we re-program LOAD,
		 * the ISR is triggered immediately after clock_set_timeout()
		 * returns; in that case we shall not increment the current_cycle
		 * because the value has been updated before LOAD re-program.
		 *
		 * We can assess if this is the case by inspecting COUNTFLAG.
		 */

		dticks = (current_cycle - announced_cycle) / CYC_PER_TICK;
		announced_cycle += dticks * CYC_PER_TICK;
		update_timelist(dticks);
	} 
	else
	{
		update_timelist(1);
	}
	
	arm_exc_exit();
}

void clock_init(void)
{
	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	
	lastload_cycle = CYC_PER_TICK - 1;
	overflowed_cycle = 0U;
	
	SysTick->LOAD =  lastload_cycle;
	SysTick->VAL  =  0; /* resets timer to lastload_cycle */
	SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk |
			  		  SysTick_CTRL_CLKSOURCE_Msk);
}

void clock_set_timeout(sword_t ticks, bool idle)
{
	/* Fast CPUs and a 24 bit counter mean that even idle systems
	 * need to wake up multiple times per second.  If the kernel
	 * allows us to miss tick announcements in idle, then shut off
	 * the counter. (Note: we can assume if idle==true that
	 * interrupts are already disabled)
	 */

	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle && ticks == FOREVER) 
	{
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		lastload_cycle = CLOSING_COUNTS;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	u32_t delay;

	ticks = (ticks == FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (sword_t)MAX_TICKS), 0);

	LOCKED(&time_lock)
	{
		u32_t pending = elapsed();
		
		current_cycle += pending;
		overflowed_cycle = 0U;
	
		u32_t unannounced = current_cycle - announced_cycle;
	
		if ((sword_t)unannounced < 0) 
		{
			/* We haven't announced for more than half the 32-bit
			 * wrap duration, because new timeouts keep being set
			 * before the existing one fires.  Force an announce
			 * to avoid loss of a wrap event, making sure the
			 * delay is at least the minimum delay possible.
			 */
			lastload_cycle = MIN_DELAY;
		} 
		else 
		{
			/* Desired delay in the future */
			delay = ticks * CYC_PER_TICK;
	
			/* Round delay up to next tick boundary */
			delay += unannounced;
			delay =  ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;
			delay -= unannounced;
			delay = MAX(delay, MIN_DELAY);
			if (delay > MAX_CYCLES) 
			{
				lastload_cycle = MAX_CYCLES;
			} 
			else 
			{
				lastload_cycle = delay;
			}
		}
		
		SysTick->LOAD = lastload_cycle - 1;
		SysTick->VAL = 0; /* resets timer to lastload_cycle */
	}
#endif
}

u32_t clock_elapsed(void)
{
	u32_t cyc = 0;
	
	if (!TICKLESS) 
	{
		return 0;
	}

	LOCKED(&time_lock)
    {
		cyc = elapsed() + current_cycle - announced_cycle;
	}
	
	return cyc / CYC_PER_TICK;
}

u32_t clock_cycle_get_32(void)
{
	u32_t ret = 0;
	
	LOCKED(&time_lock)
	{
		ret = elapsed() + current_cycle;
	}
	
	return ret;
}

void clock_idle_exit(void)
{
	if (lastload_cycle == CLOSING_COUNTS)
	{
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}
}

void clock_disable(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

void smp_timer_init()
{

}
