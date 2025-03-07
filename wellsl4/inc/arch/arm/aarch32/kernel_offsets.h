/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_ARM_INCLUDE_AARCH32_OFFSETS_SHORT_ARCH_H_
#define ARCH_ARM_INCLUDE_AARCH32_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_basepri \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_basepri_OFFSET)

#define _thread_offset_to_swap_return_value \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_swap_return_value_OFFSET)

#define _thread_offset_to_preempt_float \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_preempt_float_OFFSET)

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FP_SHARING)
#define _thread_offset_to_mode \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_mode_OFFSET)

#ifdef CONFIG_USERSPACE
#define _thread_offset_to_priv_stack_start \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_priv_stack_start_OFFSET)
#endif
#endif

#endif
