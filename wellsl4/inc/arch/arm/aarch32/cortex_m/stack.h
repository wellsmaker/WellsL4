/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-M CPUs
 *
 * Stack helper functions.
 */

#ifndef ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_STACH_
#define ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_STACH_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/arm/aarch32/cortex_m/cmsis.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-M, the interrupt stack is registered in the MSP (main stack
 * pointer) register, and switched to automatically when taking an exception.
 *
 * @return N/A
 */
static inline void arm_interrupt_stack_setup(struct interrupt_stack *int_stack)
{
	u32_t msp = (u32_t)(ARCH_THREAD_STACK_BUFFER(int_stack)) +
			    ARCH_THREAD_STACK_SIZEOF(int_stack);

	__set_MSP(msp);
#if defined(CONFIG_BUILTIN_STACK_GUARD)
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_MSPLIM((u32_t)int_stack);
#else
#error "Built-in MSP limit checks not supported by HW"
#endif
#endif

#if defined(CONFIG_STACK_ALIGN_DOUBLE_WORD)
	/* Enforce double-word stack alignment on exception entry
	 * for Cortex-M3 and Cortex-M4 (ARMv7-M) MCUs. For the rest
	 * of ARM Cortex-M processors this setting is enforced by
	 * default and it is not configurable.
	 */
#if defined(CONFIG_CPU_CORTEX_M3) || defined(CONFIG_CPU_CORTEX_M4)
	SCB->CCR |= SCB_CCR_STKALIGN_Msk;
#endif
#endif
}

#ifdef __cplusplus
}
#endif

#endif

#endif
