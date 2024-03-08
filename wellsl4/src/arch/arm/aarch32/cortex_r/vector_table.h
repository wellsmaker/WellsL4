
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

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <toolchain.h>
#include <linker/sections.h>
#include <sys/util.h>

GTEXT(__start)
GTEXT(_vector_table)

GTEXT(arm_nmi)
GTEXT(arm_undef_instruction)
GTEXT(arm_svc)
GTEXT(arm_prefetch_abort)
GTEXT(arm_data_abort)

GTEXT(arm_pendsv)
GTEXT(arm_reserved)

GTEXT(arm_prep_c)
GTEXT(isr_wrapper)

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vector_table[];

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _VECTOR_TABLE__H_ */
