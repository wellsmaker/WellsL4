
/**
 * @file
 * @brief Populated vector table in ROM
 *
 * Vector table at the beginning of the image for starting system. The reset
 * vector is the system entry point, ie. the first instruction executed.
 *
 * The table is populated with all the system exception handlers. The NMI vector
 * must be populated with a valid handler since it can happen at any time. The
 * rest should not be triggered until the kernel is ready to handle them.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GDATA(_main_stack)

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)

    /*
     * setting the _very_ early boot on the main stack allows to use memset
     * on the interrupt stack when CONFIG_INIT_STACKS is enabled before
     * switching to the interrupt stack for the rest of the early boot
     */
    .word _main_stack + CONFIG_MAIN_STACK_SIZE

    .word arm_reset
    .word arm_nmi

    .word arm_hard_fault
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_svc
    .word arm_reserved
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    .word arm_mpu_fault
    .word arm_bus_fault
    .word arm_usage_fault
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
    .word arm_secure_fault
#else
    .word arm_reserved
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
    .word arm_reserved
    .word arm_reserved
    .word arm_reserved
    .word arm_svc
    .word arm_debug_monitor
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
    .word arm_reserved
    .word arm_pendsv
#if defined(CONFIG_SYS_CLOCK_EXISTS)
    .word clock_isr
#else
    .word arm_reserved
#endif

