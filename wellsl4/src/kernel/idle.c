#include <toolchain.h>
#include <linker/sections.h>
#include <drivers/timer/system_timer.h>
#include <sys/stdbool.h>
#include <kernel/time.h>
#include <state/statedata.h>
#include <kernel/thread.h>

#if defined(CONFIG_TICKLESS_IDLE_THRESH) 
#define IDLE_THREAD_MIN_TICKS CONFIG_TICKLESS_IDLE_THRESH
#else
#define IDLE_THREAD_MIN_TICKS 1
#endif

/* Fallback idle spin loop for SMP platforms without a working IPI */
#if (defined(CONFIG_SMP) && !defined(CONFIG_SCHED_IPI_SUPPORTED))
#define SMP_IPI_NOT_SUPPORT 1
#else
#define SMP_IPI_NOT_SUPPORT 0
#endif

/**
 * @brief Make the CPU idle.
 *
 * This function makes the CPU idle until an event wakes it up.
 *
 * In a regular system, the idle thread should be the only thread responsible
 * for making the CPU idle and triggering any type of power management.
 * However, in some more constrained systems, such as a single-threaded system,
 * the only thread would be responsible for this if needed.
 *
 * @return N/A
 * @req K-CPU-IDLE-001
 */
static FORCE_INLINE void cpu_idle(void)
{
	arch_cpu_idle();
}

/**
 * @brief Make the CPU idle in an atomic fashion.
 *
 * Similar to cpu_idle(), but called with interrupts locked if operations
 * must be done atomically before making the CPU idle.
 *
 * @param key Interrupt locking key obtained from irq_lock().
 *
 * @return N/A
 * @req K-CPU-IDLE-002
 */
static FORCE_INLINE void cpu_atomic_idle(word_t key)
{
	arch_cpu_atomic_idle(key);
}

#if defined(CONFIG_SYS_POWER_MANAGEMENT) 
/*
 * Used to allow _sys_suspend() implementation to control notification
 * of the event that caused exit from kernel idling after pm operations.
 */
static byte_t sys_pm_idle_exit;

/* LCOV_EXCL_START
 * These are almost certainly overidden and in any event do nothing
 */
#if defined(CONFIG_SYS_POWER_SLEEP_STATES)
void __WEAK _sys_resume(void) 
{
}

void __WEAK _sys_suspend(void) 
{
}

#endif
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES)
void __WEAK _sys_resume_from_deep_sleep(void)
{
}
#endif
#endif

/**
 *
 * @brief Indicate that kernel is idling in tickless mode
 *
 * Sets the kernel data structure idle field to either a positive value or
 * FOREVER.
 *
 * @param ticks the number of ticks to idle
 *
 * @return N/A
 */
#if(!SMP_IPI_NOT_SUPPORT) 
static void set_kernel_idle_ticks(word_t ticks)
{
#if defined(CONFIG_SYS_POWER_MANAGEMENT) 
	_kernel.idle_ticks = ticks;
#endif
}

void sys_set_power_idle(void)
{
	word_t ticks = get_next_timelist();

	/* The documented behavior of CONFIG_TICKLESS_IDLE_THRESH is
	 * that the system should not enter a tickless idle for
	 * periods less than that.  This seems... silly, given that it
	 * saves no power and does not improve latency.  But it's an
	 * API we need to honor...
	 */
#if defined(CONFIG_SYS_CLOCK_EXISTS) 
	set_next_timelist((ticks < IDLE_THREAD_MIN_TICKS) ? 1 : ticks, true);
#endif

	set_kernel_idle_ticks(ticks);

#if (defined(CONFIG_SYS_POWER_SLEEP_STATES) || \
	defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES))
	sys_pm_idle_exit = true;

	/*
	 * Call the suspend hook function of the soc interface to allow
	 * entry into a low power state. The function returns
	 * SYS_POWER_STATE_ACTIVE if low power state was not entered, in which
	 * case, kernel does normal idle processing.
	 *
	 * This function is entered with interrupts disabled. If a low power
	 * state was entered, then the hook function should enable inerrupts
	 * before exiting. This is because the kernel does not do its own idle
	 * processing in those cases i.e. skips cpu_idle(). The kernel's
	 * idle processing re-enables interrupts which is essential for
	 * the kernel's scheduling logic.
	 */
	if (_sys_suspend(ticks) == SYS_POWER_STATE_ACTIVE)
	{
		sys_pm_idle_exit = false;
		cpu_idle();
	}
#else
	cpu_idle();
#endif
}
#endif

void sys_set_power_and_idle_exit(word_t ticks)
{
#if defined(CONFIG_SYS_POWER_SLEEP_STATES)
	/* Some CPU low power states require notification at the ISR
	 * to allow any operations that needs to be done before kernel
	 * switches task or processes int_nest_count interrupts. This can be
	 * disabled by calling _sys_pm_idle_exit_notification_disable().
	 * Alternatively it can be simply ignored if not required.
	 */
	if (sys_pm_idle_exit) 
	{
		_sys_resume();
		_sys_resume_from_deep_sleep();
	}
#endif

	clock_idle_exit();
}

/* Any task switch caused by the arrival event can exit idle */
void idle_thread_entry(void *unused1, void *unused2, void *unused3)
{

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	/* Note here: During the debugging process, 
		the low-power mode of the IDLE task will cause 
		OPENOCD to fail (this failure will be in the 
		form of a debugging communication error), 
		if the corresponding enable is not set. */
#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	extern u32_t timestamp_idle;
	timestamp_idle = get_cycle_32();
#endif

	while (true) 
	{
#if SMP_IPI_NOT_SUPPORT
		thread_busy_wait(100);
		clock_idle_exit();
#else
		(void)arch_irq_lock();
		sys_set_power_idle();
		
		if (smp_idle_domain())
		{
			sys_set_power_and_idle_exit(0);
		}
#endif
	}
}
