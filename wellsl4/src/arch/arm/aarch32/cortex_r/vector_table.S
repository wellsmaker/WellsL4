
/**
 * @file
 * @brief Populated vector table in ROM
 */

#include <toolchain.h>
#include <linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	ldr pc, =arm_reset             /*                   offset 0 */
	ldr pc, =arm_undef_instruction /* undef instruction offset 4 */
	ldr pc, =arm_svc               /* svc               offset 8 */
	ldr pc, =arm_prefetch_abort    /* prefetch abort offset  0xc */
	ldr pc, =arm_data_abort        /* data abort     offset 0x10 */
	nop				 /*                offset 0x14 */
	ldr pc, =_isr_wrapper		 /* IRQ            offset 0x18 */
	ldr pc, =arm_nmi               /* FIQ            offset 0x1c */
