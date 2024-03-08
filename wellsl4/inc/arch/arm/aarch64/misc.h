/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public kernel miscellaneous
 *
 * ARM64-specific kernel miscellaneous interface. Included by
 * arm/aarch64/arch.h.
 */

#ifndef ARCH_ARM_AARCH64_MISC_H_
#define ARCH_ARM_AARCH64_MISC_H_

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

static FORCE_INLINE void arch_kernel_init(void)
{
}

#if(0)
static FORCE_INLINE void
arch_thread_swap_retval_set(struct ktcb *thread, u32_t value)
{
	thread->arch.swap_return_value = value;
}
#endif

#if defined(CONFIG_IRQ_OFFLOAD)
extern void arm64_offload(void);
#endif

extern void arm64_call_svc(void);

extern void arm64_fatal_error(const arch_esf_t *esf, u32_t reason);
#endif

#ifdef __cplusplus
}
#endif

#endif
