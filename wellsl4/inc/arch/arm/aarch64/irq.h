/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public interrupt handling
 *
 * ARM64-specific kernel interrupt handling interface.
 * Included by arm/aarch64/arch.h.
 */

#ifndef ARCH_ARM_AARCH64_IRQ_H_
#define ARCH_ARM_AARCH64_IRQ_H_

#include <arch/irq.h>
#include <sys/stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(arch_irq_enable)
GTEXT(arch_irq_disable)
GTEXT(arch_irq_is_enabled)
#else
extern void arch_irq_enable(word_t irq);
extern void arch_irq_disable(word_t irq);
extern sword_t arch_irq_is_enabled(word_t irq);

/* internal routine documented in C file, needed by IRQ_DYNC_CONNECT() macro */
extern void arm64_irq_sched_prior_set(word_t irq, word_t prio, u32_t flag);

/* All arguments must be computable by the compiler at build time.
 *
 * ISR_DEFINE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the sched_prior in the interrupt controller at
 * runtime.
 */
#define ARCH_IRQ_DYNC_CONNECT(irq_p, sched_prior_p, isr_p, isr_param_p, flags_p) \
({ \
	ISR_DEFINE(irq_p, isr_dync_isr, isr_p, isr_param_p); \
	arm64_irq_sched_prior_set(irq_p, sched_prior_p, flags_p); \
	irq_p; \
})

#define ARCH_IRQ_DIRECT_CONNECT(irq_p, sched_prior_p, isr_p, flags_p) \
({ \
	ISR_DEFINE(irq_p, isr_direct_isr, isr_p, NULL); \
	arm64_irq_sched_prior_set(irq_p, sched_prior_p, flags_p); \
	irq_p; \
})

static FORCE_INLINE bool arch_is_in_isr(void)
{
	return _kernel.int_nest_count != 0U;
}

/* Spurious interrupt handler. Throws an error if called */
extern void irq_spurious(void *unused);

#ifdef CONFIG_GEN_SW_ISR_TABLE
/* Architecture-specific common entry point for interrupts from the vector
 * table. Most likely implemented in assembly. Looks up the correct handler
 * and parameter from the sw_isr_table and executes it.
 */
extern void isr_wrapper(void);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
