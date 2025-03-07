/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_ARM_AARCH64_TIMER_H_
#define ARCH_ARM_AARCH64_TIMER_H_

#ifndef _ASMLANGUAGE

#include <drivers/timer/arm_arch_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARM_ARCH_TIMER_IRQ	((ARM_TIMER_VIRTUAL_IRQ + 1) << 8)
#define CNTV_CTL_ENABLE		((1) << 0)


static FORCE_INLINE void arm_arch_timer_set_compare(u64_t val)
{
	__asm__ volatile("msr cntv_cval_el0, %0\n\t" : : "r" (val) : "memory");
}

static FORCE_INLINE void arm_arch_timer_enable(unsigned char enable)
{
	u32_t cntv_ctl;

	__asm__ volatile("mrs %0, cntv_ctl_el0\n\t" : "=r" (cntv_ctl) :  : "memory");

	if (enable)
		cntv_ctl |= CNTV_CTL_ENABLE;
	else
		cntv_ctl &= ~CNTV_CTL_ENABLE;

	__asm__ volatile("msr cntv_ctl_el0, %0\n\t" : : "r" (cntv_ctl) : "memory");
}

static FORCE_INLINE u64_t arm_arch_timer_count(void)
{
	u64_t cntvct_el0;

	__asm__ volatile("mrs %0, cntvct_el0\n\t" : "=r" (cntvct_el0) : : "memory");

	return cntvct_el0;
}

#ifdef __cplusplus
}
#endif

#endif

#endif
