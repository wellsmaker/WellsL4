/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A public exception handling
 *
 * ARM-specific kernel exception handling interface. Included by arm64/arch.h.
 */

#ifndef ARCH_ARM_AARCH64_EXC_H_
#define ARCH_ARM_AARCH64_EXC_H_

/* for assembler, only works with constants */


#ifdef _ASMLANGUAGE

/* nothing */

#else

#ifdef __cplusplus
extern "C" {
#endif

struct esf {
	struct basic_esf {
		u64_t regs[20];
	} basic;
};

typedef struct esf arch_esf_t;
typedef struct esf _esf_t;
typedef struct basic_esf _basic_sf_t;

#ifdef __cplusplus
}
#endif

#endif
#endif
