/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
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

#ifndef ARCH_ARM_AARCH64_THREAD_H_
#define ARCH_ARM_AARCH64_THREAD_H_

#ifndef _ASMLANGUAGE

struct callee_save {
	union {
		struct {
			u64_t x19;
			u64_t x20;
			u64_t x21;
			u64_t x22;
			u64_t x23;
			u64_t x24;
			u64_t x25;
			u64_t x26;
		};
		u64_t mr[8];
	};

	u64_t x27;
	u64_t x28;
	u64_t x29; /* FP */
	u64_t x30; /* LR */
	u64_t spsr;
	u64_t elr;
	u64_t sp;
};

typedef struct callee_save callee_save_t;

struct thread_arch {
	u32_t swap_return_value;
};

typedef struct thread_arch thread_arch_t;

#endif

#endif
