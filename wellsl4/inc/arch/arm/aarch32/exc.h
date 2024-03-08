/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public exception handling
 *
 * ARM-specific kernel exception handling interface. Included by arm/arch.h.
 */

#ifndef ARCH_ARM_AARCH32_EXC_H_
#define ARCH_ARM_AARCH32_EXC_H_

#include <generated_dts_board.h>
/* for assembler, only works with constants */
#define EXC_PRIO(pri) (((pri) << (8 - DT_NUM_IRQ_PRIO_BITS)) & 0xff)

/*
 * In architecture variants with non-programmable fault exceptions
 * (e.g. Cortex-M Baseline variants), hardware ensures processor faults
 * are given the highest interrupt sched_prior level. SVCalls are assigned
 * the highest configurable sched_prior level (level 0); note, however, that
 * this interrupt level may be shared with HW interrupts.
 *
 * In Cortex variants with programmable fault exception priorities we
 * assign the highest interrupt sched_prior level (level 0) to processor faults
 * with configurable sched_prior.
 * The highest sched_prior level may be shared with either Zero-Latency IRQs (if
 * support for the feature is enabled) or with SVCall sched_prior level.
 * Regular HW IRQs are always assigned sched_prior levels lower than the sched_prior
 * levels for SVCalls, Zero-Latency IRQs and processor faults.
 *
 * PendSV IRQ (which is used in Cortex-M variants to implement thread
 * context-switching) is assigned the lowest IRQ sched_prior level.
 */
#if defined(CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS)
#define _EXCEPTION_RESERVED_PRIO 1
#else
#define _EXCEPTION_RESERVED_PRIO 0
#endif

#define _EXC_FAULT_PRIO 0

#ifdef CONFIG_ZERO_LATENCY_IRQS
#define _EXC_ZERO_LATENCY_IRQS_PRIO 0
#define _EXC_SVC_PRIO 1
#define _IRQ_PRIO_OFFSET (_EXCEPTION_RESERVED_PRIO + 1)
#else
#define _EXC_SVC_PRIO 0
#define _IRQ_PRIO_OFFSET (_EXCEPTION_RESERVED_PRIO)
#endif

#define _EXC_IRQ_DEFAULT_PRIO EXC_PRIO(_IRQ_PRIO_OFFSET)

/* Use lowest possible sched_prior level for PendSV */
#define _EXC_PENDSV_PRIO 0xff
#define _EXC_PENDSV_PRIO_MASK EXC_PRIO(_EXC_PENDSV_PRIO)

#ifdef _ASMLANGUAGE
GTEXT(arm_exc_exit);
#else

#ifdef __cplusplus
extern "C" {
#endif

/* ARM GPRs are often designated by two different names */
#define sys_alias_gprs(name1, name2) union { u32_t name1, name2; }

struct esf {
	struct basic_esf {
		sys_alias_gprs(a1, r0);
		sys_alias_gprs(a2, r1);
		sys_alias_gprs(a3, r2);
		sys_alias_gprs(a4, r3);
		sys_alias_gprs(ip, r12);
		sys_alias_gprs(lr, r14);
		sys_alias_gprs(pc, r15);
		u32_t xpsr;
	} basic;
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	float s[16];
	u32_t fpscr;
	u32_t undefined;
#endif
};

typedef struct esf arch_esf_t;
typedef struct esf _esf_t;
typedef struct basic_esf _basic_sf_t;

extern void arm_exc_exit(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
