/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M public kernel miscellaneous
 *
 * ARM-specific kernel miscellaneous interface. Included by arm/arch.h.
 */

#ifndef ARCH_ARM_AARCH32_MISC_H_
#define ARCH_ARM_AARCH32_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern u32_t clock_cycle_get_32(void);

static FORCE_INLINE u32_t arch_k_cycle_get_32(void)
{
	return clock_cycle_get_32();
}

static FORCE_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

extern void arm_fault_init(void);
extern void arm_cpu_idle_init(void);

static FORCE_INLINE void arch_kernel_init(struct interrupt_stack *int_stack)
{
	arm_interrupt_stack_setup(int_stack);
	arm_exc_setup();
	arm_fault_init();
	arm_cpu_idle_init();
	arm_clear_faults();
}
#if(0)
static FORCE_INLINE void
arch_thread_swap_retval_set(struct ktcb *thread, u32_t value)
{
	thread->arch.swap_return_value = value;
}
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
