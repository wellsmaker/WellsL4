
/**
 * @file
 * @brief ARM Cortex-R System Control Block interface
 */

#include <arch/cpu.h>
#include <sys/util.h>

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * @return N/A
 */

void __weak sys_arch_reboot(sword_t type)
{
	ARG_UNUSED(type);
}
