/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ARCH_ARM_AARCH64_ASM_INLINE_H_
#define ARCH_ARM_AARCH64_ASM_INLINE_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

static FORCE_INLINE word_t arch_get_curr_cpu_index(void)
{
	return 0;
}

static FORCE_INLINE word_t arch_irq_lock(void)
{
	word_t key;

	/*
	 * Return the whole DAIF register as key but use DAIFSET to disable
	 * IRQs.
	 */
	__asm__ volatile("mrs %0, daif;"
			 "msr daifset, %1;"
			 "isb"
			 : "=r" (key)
			 : "i" (DAIFSET_IRQ)
			 : "memory", "cc");

	return key;
}

static FORCE_INLINE void arch_irq_unlock(word_t key)
{
	__asm__ volatile("msr daif, %0;"
			 "isb"
			 :
			 : "r" (key)
			 : "memory", "cc");
}

static FORCE_INLINE bool arch_irq_unlocked(word_t key)
{
	/* We only check the (I)RQ bit on the DAIF register */
	return (key & DAIF_IRQ) == 0;
}

#ifdef __cplusplus
}
#endif

#endif
#endif
