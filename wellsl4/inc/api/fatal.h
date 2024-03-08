#ifndef FATAL_H
#define FATAL_H

#include <arch/cpu.h>
#include <toolchain.h>
#include <kernel_object.h>


#ifdef __cplusplus
extern "C" {
#endif


enum fatal_type {
	/** Generic CPU exception, not covered by other codes */
	fatal_cpu_exception_fatal,
	
	/** Unhandled hardware interrupt */
	fatal_spurious_irq_fatal,

	/** Faulting context overflowed its stack buffer */
	fatal_stack_check_fatal,

	/** Moderate severity software error */
	fatal_oops_fatal,

	/** High severity software error */
	fatal_panic_fatal,

	/* TODO: add more codes for exception types that are common across
	 * architectures
	 */
};

/**
 * @brief Halt the system on a fatal error
 *
 * Invokes architecture-specific code to power off or halt the system in
 * a low power state. Lacking that, lock interrupts and sit in an idle loop.
 *
 * @param reason Fatal exception reason code
 */
FUNC_NORETURN void fatal_halt(word_t reason);

/**
 * @brief Fatal error policy handler
 *
 * This function is not invoked by application code, but is declared as a
 * weak symbol so that applications may introduce their own policy.
 *
 * The default implementation of this function halts the system
 * unconditionally. Depending on architecture support, this may be
 * a simple infinite loop, power off the hardware, or exit an emulator.
 *
 * If this function returns, then the currently executing thread will be
 * aborted.
 *
 * A few notes for custom implementations:
 *
 * - If the error is determined to be unrecoverable, LOG_PANIC() should be
 *   invoked to flush any pending logging buffers.
 * - fatal_panic_fatal indicates a severe unrecoverable error in the kernel
 *   itself, and should not be considered recoverable. There is an assertion
 *   in fatal_error() to enforce this.
 * - Even outside of a kernel panic, unless the fault occurred in user mode,
 *   the kernel itself may be in an inconsistent state, with API calls to
 *   kernel k_objects possibly exhibiting undefined behavior or triggering
 *   another exception.
 *
 * @param reason The reason for the fatal error
 * @param esf Exception context, with details and partial or full register
 *            state when the error occurred. May in some cases be NULL.
 */
void system_halt_handler(word_t reason, const arch_esf_t *esf);

/**
 * Called by architecture code upon a fatal error.
 *
 * This function dumps out architecture-agnostic information about the error
 * and then makes a policy decision on what to do by invoking
 * system_halt_handler().
 *
 * On architectures where thread_abort() never returns, this function
 * never returns either.
 *
 * @param reason The reason for the fatal error
 * @param esf Exception context, with details and partial or full register
 *            state when the error occurred. May in some cases be NULL.
 */
void fatal_error(word_t reason, const arch_esf_t *esf);


#ifdef ARCH_CATCH_EXCEPTION
/* This architecture has direct support for triggering a CPU exception */
#define catch_exception(cond) 	ARCH_CATCH_EXCEPTION(cond)
#else

/* NOTE: This is the implementation for arches that do not implement
 * ARCH_CATCH_EXCEPTION() to generate a real CPU exception.
 *
 * We won't have a real exception page_f to determine the PC value when
 * the oops occurred, so print file and line number before we jump into
 * the fatal error handler.
 */
#define catch_exception(cond) do { \
		printk("@ %s:%d:\n", __FILE__,  __LINE__); \
		fatal_error(cond, NULL); } while (false)

#endif

/**
 * @brief Fatally terminate a thread
 *
 * This should be called when a thread has encountered an unrecoverable
 * runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code fatal_oops_fatal.
 *
 * If this is called from ISR context, the default system fatal error handler
 * will treat it as an unrecoverable system error, just like k_panic().
 * @req K-MISC-003
 */
#define k_oops() catch_exception(fatal_oops_fatal)

/**
 * @brief Fatally terminate the system
 *
 * This should be called when the WellL4 kernel has encountered an
 * unrecoverable runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code fatal_panic_fatal.
 * @req K-MISC-004
 */
#define k_panic()	catch_exception(fatal_panic_fatal)
#define k_cpu_exception()	 catch_exception(fatal_cpu_exception_fatal)
#define k_spurious_irq() catch_exception(fatal_spurious_irq_fatal)
#define k_stack_check() catch_exception(fatal_stack_check_fatal)

#ifdef __cplusplus
}
#endif

#endif
