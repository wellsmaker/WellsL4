/**
 * @file
 * @brief Fault handlers for ARM Cortex-M and Cortex-R
 *
 * Fault handlers for ARM Cortex-M and Cortex-R processors.
 */

#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(arm_fault)

GTEXT(arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(arm_mpu_fault)
GTEXT(arm_bus_fault)
GTEXT(arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE*/
GTEXT(arm_debug_monitor)
#elif defined(CONFIG_ARMV7_R)
GTEXT(arm_undef_instruction)
GTEXT(arm_prefetch_abort)
GTEXT(arm_data_abort)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(arm_reserved)

/**
 *
 * @brief Fault handler installed in the fault and reserved vectors
 *
 * Entry point for the HardFault, MemManageFault, BusFault, UsageFault,
 * SecureFault, Debug Monitor, and reserved exceptions.
 *
 * For Cortex-M: the function supplies the values of
 * - the MSP
 * - the PSP
 * - the EXC_RETURN value
 * as parameters to the arm_fault() C function that will perform the
 * rest of the fault handling (i.e. arm_fault(MSP, PSP, EXC_RETURN)).
 *
 * For Cortex-R: the function simply invokes arm_fault() with currently
 * unused arguments.
 *
 * Provides these symbols:
 *
 *   arm_hard_fault
 *   arm_mpu_fault
 *   arm_bus_fault
 *   arm_usage_fault
 *   arm_secure_fault
 *   arm_debug_monitor
 *   arm_reserved
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_mpu_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_bus_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_debug_monitor)
#elif defined(CONFIG_ARMV7_R)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_undef_instruction)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_prefetch_abort)
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_data_abort)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
SECTION_SUBSEC_FUNC(TEXT,__fault,arm_reserved)

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || \
	defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	mrs r0, MSP
	mrs r1, PSP
	mov r2, lr /* EXC_RETURN */

	push {r0, lr}
#elif defined(CONFIG_ARMV7_R)
	/*
	 * Pass null for the esf to arm_fault for now. A future PR will add
	 * better exception debug for Cortex-R that subsumes what esf
	 * provides.
	 */
	mov r0, #0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE || CONFIG_ARMv7_M_ARMV8_M_MAINLINE */

	bl arm_fault

#if defined(CONFIG_CPU_CORTEX_M)
	pop {r0, pc}
#elif defined(CONFIG_CPU_CORTEX_R)
	pop {r0, lr}
	subs pc, lr, #8
#endif

	.end
