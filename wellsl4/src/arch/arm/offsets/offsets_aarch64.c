/**
 * @file
 * @brief ARM64 kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various ARM kernel structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final kernel ELF image (due to the linker's reference to the _OffsetAbsSyms
 * symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

#include <object/offsets_macro.h>
#include <object/offsets_kernel.h>
#include <arch/cpu.h>

GEN_OFFSET_SYM(thread_arch_t, swap_return_value);
GEN_ABS_SYM_END


