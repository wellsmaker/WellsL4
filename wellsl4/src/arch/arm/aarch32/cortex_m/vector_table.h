

/**
 * @file
 * @brief Definitions for the boot vector table
 *
 *
 * Definitions for the boot vector table.
 *
 * System exception handler names all have the same format:
 *
 *   __<exception name with underscores>
 *
 * No other symbol has the same format, so they are easy to spot.
 */

#ifndef ARCH_ARM_CORE_AARCH32_CORTEX_M_VECTOR_TABLE_H_
#define ARCH_ARM_CORE_AARCH32_CORTEX_M_VECTOR_TABLE_H_

#ifdef _ASMLANGUAGE

#include <toolchain.h>
#include <linker/sections.h>
#include <sys/util.h>


GTEXT(__start)
GTEXT(_vector_table)

GTEXT(arm_reset)
GTEXT(arm_nmi)
GTEXT(arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
GTEXT(arm_svc)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(arm_mpu_fault)
GTEXT(arm_bus_fault)
GTEXT(arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
GTEXT(arm_svc)
GTEXT(arm_debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(arm_pendsv)
GTEXT(arm_reserved)

GTEXT(arm_prep_c)
GTEXT(isr_wrapper)

#if defined(CONFIG_SYS_CLOCK_EXISTS)
GTEXT(clock_isr)
#endif

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vector_table[];

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ARCH_ARM_CORE_AARCH32_CORTEX_M_VECTOR_TABLE_H_ */
