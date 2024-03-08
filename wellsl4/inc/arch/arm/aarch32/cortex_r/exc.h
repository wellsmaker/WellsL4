/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception/interrupt context helpers for Cortex-R CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef _ARM_CORTEXR_ISR__H_
#define _ARM_CORTEXR_ISR__H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/irq.h>
#include <arch/arm/aarch32/cortex_r/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_OFFLOAD
extern volatile irq_offload_routine_t offload_routine;
#endif

/* Check the CPSR mode bits to see if we are in IRQ or FIQ mode */
static FORCE_INLINE bool arch_is_in_isr(void)
{
	word_t status;

	__asm__ volatile(
			" mrs %0, cpsr"
			: "=r" (status) : : "memory", "cc");
	status &= MODE_MASK;

	return	(status == MODE_FIQ) ||
		(status == MODE_IRQ) ||
		(status == MODE_SVC);
}

/**
 * @brief Setup system exceptions
 *
 * Enable fault exceptions.
 *
 * @return N/A
 */
static FORCE_INLINE void arm_exc_setup(void)
{
}

/**
 * @brief Clear Fault exceptions
 *
 * Clear out exceptions for Mem, Bus, Usage and Hard Faults
 *
 * @return N/A
 */
static FORCE_INLINE void arm_clear_faults(void)
{
}

extern void arm_cortex_r_svc(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
