/**
 * @file
 * @brief Thread entry
 *
 * This file provides the common thread entry function
 */

#include <state/statedata.h>
#include <object/tcb.h>


/*
 * Common thread entry point function (used by all record_threads)
 *
 * This routine invokes the actual thread entry point function and passes
 * it three arguments. It also handles graceful termination of the thread
 * if the entry point function ever returns.
 *
 * This routine does not return, and is marked as such so the compiler won't
 * generate preamble code that is only used by functions that actually return.
 */
FUNC_NORETURN void thread_entry_point(ktcb_entry_t entry,
	void *p1, void *p2, void *p3)
{
	entry(p1, p2, p3);

	thread_abort(_current_thread);

	/*
	 * Compiler can't tell that thread_abort() won't return and issues a
	 * warning unless we tell it that control never gets this far.
	 */

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}
