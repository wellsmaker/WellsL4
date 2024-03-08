/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 specific kernel interface header
 *
 * This header contains the ARM64 specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arm/aarch64/cpu.h)
 */

#ifndef ARCH_ARM_AARCH64_ARCH_H_
#define ARCH_ARM_AARCH64_ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declare the THREAD_STACK_ALIGN_SIZE
 *
 * Denotes the required alignment of the stack pointer on public API
 * boundaries
 *
 */
#define STACK_ALIGN		16
#define THREAD_STACK_ALIGN_SIZE	STACK_ALIGN

#include <arch/arm/aarch64/thread.h>
#include <kernel_object.h>
#include <arch/arm/aarch64/exc.h>
#include <arch/arm/aarch64/irq.h>
#include <arch/arm/aarch64/misc.h>
#include <arch/arm/aarch64/cpu.h>
#include <arch/arm/aarch64/arch_inlines.h>
#include <arch/arm/aarch64/sys_io.h>
#include <arch/arm/aarch64/timer.h>

#ifdef __cplusplus
}
#endif
#endif
