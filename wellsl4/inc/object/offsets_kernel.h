/*
 * Copyright (c) 2010, 2012, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Macros to generate structure member offset definitions
 *
 * This header contains macros to allow a kernel implementation to generate
 * absolute symbols whose values represents the member offsets for various
 * kernel structures.  These absolute symbols are typically utilized by
 * assembly source files rather than hardcoding the values in some local header
 * file.
 *
 * WARNING: Absolute symbols can potentially be utilized by external tools --
 * for example, to locate a specific field within a data structure.
 * Consequently, changes made to such symbols may require modifications to the
 * associated tool(s). Typically, relocating a member of a structure merely
 * requires that a tool be rebuilt; however, moving a member to another
 * structure (or to a new sub-structure within an existing structure) may
 * require that the tool itself be modified. Likewise, deleting, renaming, or
 * changing the meaning of an absolute symbol may require modifications to a
 * tool.
 *
 * The macro "GEN_OFFSET_SYM(structure, member)" is used to generate a single
 * absolute symbol.  The absolute symbol will appear in the object module
 * generated from the source file that utilizes the GEN_OFFSET_SYM() macro.
 * Absolute symbols representing a structure member offset have the following
 * form:
 *
 *    __<structure>_<member>_OFFSET
 *
 * This header also defines the GEN_ABSOLUTE_SYM macro to simply define an
 * absolute symbol, irrespective of whether the value represents a structure
 * or offset.
 *
 * The following sample file illustrates the usage of the macros available
 * in this file:
 *
 *	<START of sample source file: offsets.c>
 *
 * #include <gen_offset.h>
 * /@ include struct definitions for which offsets symbols are to be
 * generated @/
 *
 * #include <kernel_structs.h>
 * GEN_ABS_SYM_BEGIN (_OffsetAbsSyms)	/@ the name parameter is arbitrary @/
 * /@ kernel_t structure member offsets @/
 *
 * GEN_OFFSET_SYM (kernel_t, int_nest_count);
 * GEN_OFFSET_SYM (kernel_t, int_stack_point);
 * GEN_OFFSET_SYM (kernel_t, _current_thread);
 * GEN_OFFSET_SYM (kernel_t, idle);
 *
 * GEN_ABSOLUTE_SYM (___kernel_t_SIZEOF, sizeof(kernel_t));
 *
 * GEN_ABS_SYM_END
 * <END of sample source file: offsets.c>
 *
 * Compiling the sample offsets.c results in the following symbols in offsets.o:
 *
 * $ nm offsets.o
 * 00000000 A ___kernel_t_irq_nested_OFFSET
 * 00000004 A ___kernel_t_irq_stack_OFFSET
 * 00000008 A ___kernel_t_current_OFFSET
 * 0000000c A ___kernel_t_idle_OFFSET
 */
#include <types_def.h>
#include <kernel_object.h>



#ifndef OFFSETS_KERNEL_H_
#define OFFSETS_KERNEL_H_


/* All of this is build time magic, but LCOV gets confused. Disable coverage
 * for this whole file.
 *
 */

/*
 * The final link step uses the symbol _OffsetAbsSyms to force the linkage of
 * offsets.o into the ELF image.
 */

GEN_ABS_SYM_BEGIN(_OffsetAbsSyms)
GEN_OFFSET_SYM(cpu_t, current_thread);
GEN_OFFSET_SYM(cpu_t, int_nest_count);
GEN_OFFSET_SYM(cpu_t, int_stack_point);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(kernel_t, monitor_thread);
#endif

#if defined(CONFIG_SYS_POWER_MANAGEMENT) 
GEN_OFFSET_SYM(kernel_t, idle_ticks);
#endif

GEN_OFFSET_SYM(kernel_t, ready_thread);

#if defined(CONFIG_FP_SHARING) 
GEN_OFFSET_SYM(kernel_t, float_thread);
#endif

GEN_ABSOLUTE_SYM(STRUCT_KERNEL_SIZE, sizeof(kernel_t));

GEN_OFFSET_SYM(thread_base_t, option);
GEN_OFFSET_SYM(thread_base_t, thread_state);
GEN_OFFSET_SYM(thread_base_t, sched_prior);
GEN_OFFSET_SYM(thread_base_t, sched_locked);
GEN_OFFSET_SYM(thread_base_t, preempt);
GEN_OFFSET_SYM(thread_base_t, swap_data);

GEN_OFFSET_SYM(ktcb_t, base);
GEN_OFFSET_SYM(ktcb_t, callee_saved);
GEN_OFFSET_SYM(ktcb_t, arch);

#if defined(CONFIG_USE_SWITCH) 
GEN_OFFSET_SYM(ktcb_t, swap_handler);
#endif

#if defined(CONFIG_THREAD_STACK_INFO) 
GEN_OFFSET_SYM(thread_stack_info_t, start);
GEN_OFFSET_SYM(thread_stack_info_t, size);

GEN_OFFSET_SYM(ktcb_t, stack_info);
#endif

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(ktcb_t, monitor_thread_next);
#endif

GEN_ABSOLUTE_SYM(THREAD_SIZEOF, sizeof(struct ktcb));

/* size of the device structure. Used by linker scripts */
GEN_ABSOLUTE_SYM(DEVICE_STRUCT_SIZEOF, sizeof(struct device));

#endif
