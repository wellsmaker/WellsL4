/* cpu.h - automatically selects the correct arch.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_CPU_H_
#define ARCH_CPU_H_


#include <types_def.h>
#include <arch/ffs.h>

#if defined(CONFIG_X86)
#include <arch/x86/arch.h>
#elif defined(CONFIG_ARM64)
#include <arch/arm/aarch64/arch.h>
#elif defined(CONFIG_ARM)
#include <arch/arm/aarch32/arch.h>
#elif defined(CONFIG_ARC)
#include <arch/arc/arch.h>
#elif defined(CONFIG_NIOS2)
#include <arch/nios2/arch.h>
#elif defined(CONFIG_RISCV)
#include <arch/riscv/arch.h>
#elif defined(CONFIG_XTENSA)
#include <arch/xtensa/arch.h>
#elif defined(CONFIG_ARCH_POSIX)
#include <arch/posix/arch.h>
#else
#error "Unknown Architecture"
#endif

#endif
