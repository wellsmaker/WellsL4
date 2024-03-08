

#include <arch/arm/exc.h>
#include <fatal.h>

/**
 *
 * @brief Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible only for reporting the error. Once reported, it then
 * invokes the user provided routine _SysFatalErrorHandler() which is
 * responsible for implementing the error handling policy.
 *
 * This is a stub for more exception handling code to be added later.
 */
void arm_fault(arch_esf_t *esf, u32_t exc_return)
{
	arm_fatal_error(fatal_cpu_exception_fatal, esf);
}

void arm_fault_init(void)
{
}
