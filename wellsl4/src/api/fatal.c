#include <types_def.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <arch/cpu.h>
#include <api/fatal.h>
#include <object/tcb.h>
#include <sys/assert.h>
#include <state/statedata.h>


extern void thread_abort(struct ktcb *thread);

FUNC_NORETURN __weak void arch_system_halt(word_t reason)
{
	ARG_UNUSED(reason);

	/* TODO: What's the best way to totally halt the system if SMP
	 * is enabled?
	 */

	(void) arch_irq_lock();
	for (;;) 
	{
		/* Spin endlessly */
	}
}

__weak void system_halt_handler(word_t reason, const arch_esf_t *esf)
{
	ARG_UNUSED(esf);
	user_error("Halting system");
	arch_system_halt(reason);
	
	CODE_UNREACHABLE;
}

#if defined(CONFIG_THREAD_NAME)
static const string thread_name_get(struct ktcb *thread)
{
	const char *thread_name = thread_get_name(thread);

	if (thread_name == NULL || thread_name[0] == '\0') 
	{
		thread_name = "unknown";
	}

	return thread_name;
}
#endif

const string reason_to_string(word_t reason)
{
	switch (reason) 
	{
		case fatal_cpu_exception_fatal:
			return "CPU exception";
		case fatal_spurious_irq_fatal:
			return "Unhandled interrupt";
		case fatal_stack_check_fatal:
			return "Stack overflow";
		case fatal_oops_fatal:
			return "Kernel oops";
		case fatal_panic_fatal:
			return "Kernel panic";
		default:
			return "Unknown error";
	}
}

byte_t get_cpu_id(void)
{
#if defined(CONFIG_SMP)
	return arch_curr_cpu()->core_id;
#else
	return 0;
#endif
}

FUNC_NORETURN void fatal_halt(word_t reason)
{
	arch_system_halt(reason);
}

void fatal_error(word_t reason, const arch_esf_t *esf)
{
	struct ktcb *thread = _current_thread;

	/* sanitycheck looks for the "WELLSL4 FATAL ERROR" string, don't
	 * change it without also updating sanitycheck
	 */
	user_error("FATAL ERROR %d: %s on CPU %d", reason, 
		reason_to_string(reason), get_cpu_id());

	/* FIXME: This doesn't seem to work as expected on all arches.
	 * Need a reliable way to determine whether the fault happened when
	 * an IRQ or exception was being handled, or thread context.
	 *
	 * See #17656
	 */
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
	if (arch_is_in_irq_nested_exception(esf)) 
	{
		user_error("Fault during interrupt handling\n");
	}
#endif

#if defined(CONFIG_THREAD_NAME)
	user_error("Current thread: %p (%s)", thread, thread_name_get(thread));
#endif

	system_halt_handler(reason, esf);

	/* If the system fatal error handler returns, then kill the faulting
	 * thread; a policy decision was made not to hang the system.
	 *
	 * Policy for fatal errors in ISRs: unconditionally panic.
	 *
	 * There is one exception to this policy: a stack sentinel
	 * check may be performed (on behalf of the _current_thread thread)
	 * during ISR exit, but in this case the thread should be
	 * aborted.
	 *
	 * Note that thread_abort() returns on some architectures but
	 * not others; e.g. on ARC, x86_64, Xtensa with ASM2, ARM
	 */
	if (!IS_ENABLED(CONFIG_TEST)) 
	{
		assert_info(reason != fatal_panic_fatal,
			 "Attempted to recover from a kernel panic condition");
		/* FIXME: #17656 */
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
		if (arch_is_in_irq_nested_exception(esf))
		{
#if defined(CONFIG_STACK_SENTINEL)
			if (reason != fatal_stack_check_fatal)
			{
				user_error("Attempted to recover from a fatal error in ISR");
			}
#endif
		}
#endif
	} 
	else
	{
#if defined(CONFIG_ARCH_HAS_NESTED_EXCEPTION_DETECTION)
		if (arch_is_in_irq_nested_exception(esf)) 
		{
			/* Abort the thread only on STACK Sentinel check fail. */
#if defined(CONFIG_STACK_SENTINEL)
			if (reason != fatal_stack_check_fatal) 
			{
				return;
			}
#else
			return;
#endif
		}
#endif
	}
	thread_abort(thread);
}
