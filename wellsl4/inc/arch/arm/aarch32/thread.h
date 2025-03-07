/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct callee_saved
  *
 * necessary to instantiate instances of struct ktcb.
 */

#ifndef ARCH_ARM_AARCH32_THREAD_H_
#define ARCH_ARM_AARCH32_THREAD_H_

#ifndef _ASMLANGUAGE

struct callee_save {
	union {
		struct  {
			u32_t v1;  /* r4 */
			u32_t v2;  /* r5 */
			u32_t v3;  /* r6 */
			u32_t v4;  /* r7 */
			u32_t v5;  /* r8 */
			u32_t v6;  /* r9 */
			u32_t v7;  /* r10 */
			u32_t v8;  /* r11 */
		};
		u32_t mr[8];
	};

#if defined(CONFIG_CPU_CORTEX_R)
	u32_t spsr;/* r12 */
	u32_t psp; /* r13 */
	u32_t lr;  /* r14 */
#else
	u32_t psp; /* r13 */
#endif
};

typedef struct callee_save callee_save_t;

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
struct pree_float {
	float  s16;
	float  s17;
	float  s18;
	float  s19;
	float  s20;
	float  s21;
	float  s22;
	float  s23;
	float  s24;
	float  s25;
	float  s26;
	float  s27;
	float  s28;
	float  s29;
	float  s30;
	float  s31;
};
#endif

struct thread_arch {

	/* interrupt locking key */
	u32_t basepri;

	/* r0 in stack page_f cannot be written to reliably */
	u32_t swap_return_value;

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	/*
	 * No cooperative floating point register set structure exists for
	 * the Cortex-M as it automatically saves the necessary registers
	 * in its exception stack page_f.
	 */
	struct pree_float  preempt_float;
#endif

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FP_SHARING)
	u32_t mode;
#if defined(CONFIG_USERSPACE)
	u32_t priv_stack_start;
#endif
#endif
};

typedef struct thread_arch thread_arch_t;


#endif
#endif
