/**
 * @file
 * @brief ARM kernel structure member offset definition file
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

GEN_OFFSET_SYM(thread_arch_t, basepri);
GEN_OFFSET_SYM(thread_arch_t, swap_return_value);

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FP_SHARING)
GEN_OFFSET_SYM(thread_arch_t, mode);
#if defined(CONFIG_USERSPACE)
GEN_OFFSET_SYM(thread_arch_t, priv_stack_start);
#endif
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
GEN_OFFSET_SYM(thread_arch_t, preempt_float);
#endif

GEN_OFFSET_SYM(_basic_sf_t, a1);
GEN_OFFSET_SYM(_basic_sf_t, a2);
GEN_OFFSET_SYM(_basic_sf_t, a3);
GEN_OFFSET_SYM(_basic_sf_t, a4);
GEN_OFFSET_SYM(_basic_sf_t, ip);
GEN_OFFSET_SYM(_basic_sf_t, lr);
GEN_OFFSET_SYM(_basic_sf_t, pc);
GEN_OFFSET_SYM(_basic_sf_t, xpsr);

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
GEN_OFFSET_SYM(_esf_t, s);
GEN_OFFSET_SYM(_esf_t, fpscr);
#endif

GEN_ABSOLUTE_SYM(___esf_t_SIZEOF, sizeof(_esf_t));

GEN_OFFSET_SYM(callee_save_t, v1);
GEN_OFFSET_SYM(callee_save_t, v2);
GEN_OFFSET_SYM(callee_save_t, v3);
GEN_OFFSET_SYM(callee_save_t, v4);
GEN_OFFSET_SYM(callee_save_t, v5);
GEN_OFFSET_SYM(callee_save_t, v6);
GEN_OFFSET_SYM(callee_save_t, v7);
GEN_OFFSET_SYM(callee_save_t, v8);
GEN_OFFSET_SYM(callee_save_t, psp);
#if defined(CONFIG_CPU_CORTEX_R)
GEN_OFFSET_SYM(callee_save_t, spsr);
GEN_OFFSET_SYM(callee_save_t, lr);
#endif

/* size of the entire preempt registers structure */

GEN_ABSOLUTE_SYM(__callee_saved_t_SIZEOF, sizeof(struct callee_save));

#if defined(CONFIG_THREAD_STACINFO)
GEN_OFFSET_SYM(struct thread_stack_info, start);
GEN_ABSOLUTE_SYM(__thread_stack_info_t_SIZEOF, sizeof(struct thread_stack_info));
#endif

/*
 * size of the struct ktcb structure sans save area for floating
 * point registers.
 */

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
GEN_ABSOLUTE_SYM(_THREAD_NO_FLOAT_SIZEOF, sizeof(struct ktcb) - sizeof(struct pree_float));
#else
GEN_ABSOLUTE_SYM(_THREAD_NO_FLOAT_SIZEOF, sizeof(struct ktcb));
#endif

GEN_ABS_SYM_END

