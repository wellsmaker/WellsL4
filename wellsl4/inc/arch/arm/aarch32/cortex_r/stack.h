/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for Cortex-R CPUs
 *
 * Stack helper functions.
 */

#ifndef _ARM_CORTEXR_STAC_H_
#define _ARM_CORTEXR_STAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern void arm_init_stacks(void);

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-R, the interrupt stack is set up by reset.S
 *
 * @return N/A
 */
static FORCE_INLINE void arm_interrupt_stack_setup(struct interrupt_stack *int_stack)
{
}

#endif

#ifdef __cplusplus
}
#endif

#endif
