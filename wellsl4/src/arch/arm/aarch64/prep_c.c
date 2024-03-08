
/**
 * @file
 * @brief Full C support initialization
 *
 * Initialization of full C support: zero the .bss and call cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel/boot.h>
#include <toolchain.h>

extern FUNC_NORETURN void cstart(void);
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */
void arm64_prep_c(void)
{
	set_bss_zero();
	cstart();

	CODE_UNREACHABLE;
}
