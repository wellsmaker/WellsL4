/**
 * @file
 * @brief Compiler stack protection (kernel part)
 *
 * This module provides functions to support compiler stack protection
 * using canaries.  This feature is enabled with configuration
 * CONFIG_STACCANARIES=y.
 *
 * When this feature is enabled, the compiler generated code refers to
 * function __stack_chk_fail and global variable __stack_chk_guard.
 */

/* stack overflow :
   To detect the destruction of function stack, it is necessary to modify 
   the organization of function stack and insert a canary word between buffer 
   and control information (such as EBP). In this way, when the buffer overflows, 
   the Canary word will be first covered before the return address is covered. 
   By checking whether the value of Canary word is modified, we can judge whether 
   the overflow attack has occurred. 
   At present, the main compiler stack protection implementations, such as stack guard
   and stack smashing protection (SSP), take canaries detection as the main protection 
   technology, but canaries are produced in different ways. */
/* canary word:
   1. Terminator canaries
   2. Random canaries
   3. Random XOR canaries */

#include <toolchain.h>
#include <api/errno.h>
/**
 *
 * @brief Referenced by GCC compiler generated code
 *
 * This routine is invoked when a stack canary error is detected, indicating
 * a buffer overflow or stack corruption problem.
 *
 * @brief Stack canary error handler
 *
 * This function is invoked when a stack canary error is detected.
 *
 * @return Does not return
 */
void stack_chk_handler(void)
{
	/* Stack canary error is a software fatal condition; treat it as such.
	 */
	catch_exception(fatal_stack_check_fatal);
	CODE_UNREACHABLE;
}
FUNC_ALIAS(stack_chk_handler, __stack_chk_fail, void);
