
/**
 * @file
 * @brief ARM Cortex-M System Control Block interface
 *
 *
 * Most of the SCB interface consists of simple bit-flipping methods, and is
 * implemented as FORCE_INLINE functions in scb.h. This module thus contains only data
 * definitions and more complex routines, if needed.
 */

#include <types_def.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

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

	NVIC_SystemReset();
}
