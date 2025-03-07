/* ARM Cortex-M GCC specific public FORCE_INLINE assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ARCH_ARM_AARCH32_ARCH_INLINE_H_
#define ARCH_ARM_AARCH32_ARCH_INLINE_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <arch/arm/aarch32/exc.h>
#include <arch/irq.h>

#if defined(CONFIG_CPU_CORTEX_R)
#include <arch/arm/cortex_r/cpu.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static FORCE_INLINE word_t arch_get_curr_cpu_index(void)
{
	return 0;
}

/* On ARMv7-M and ARMv8-M Mainline CPUs, this function prevents regular
 * exceptions (i.e. with interrupt sched_prior lower than or equal to
 * _EXC_IRQ_DEFAULT_PRIO) from interrupting the CPU. NMI, Faults, SVC,
 * and Zero Latency IRQs (if supported) may still interrupt the CPU.
 *
 * On ARMv6-M and ARMv8-M Baseline CPUs, this function reads the value of
 * PRIMASK which shows if interrupts are enabled, then disables all interrupts
 * except NMI.
 */

static FORCE_INLINE word_t arch_irq_lock(void)
{
	word_t key;

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile("mrs %0, PRIMASK;"
		"cpsid i"
		: "=r" (key)
		:
		: "memory");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	word_t tmp;

	__asm__ volatile(
		"mov %1, %2;"
		"mrs %0, BASEPRI;"
		"msr BASEPRI, %1;"
		"isb;"
		: "=r"(key), "=r"(tmp)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "memory");
#elif defined(CONFIG_ARMV7_R)
	__asm__ volatile(
		"mrs %0, cpsr;"
		"and %0, #" TOSTR(I_BIT) ";"
		"cpsid i;"
		: "=r" (key)
		:
		: "memory", "cc");
#else
#error Unknown ARM architecture
#endif
	return key;
}


/* On Cortex-M0/M0+, this enables all interrupts if they were not
 * previously disabled.
 */

static FORCE_INLINE void arch_irq_unlock(word_t key)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	if (key) {
		return;
	}
	__asm__ volatile(
		"cpsie i;"
		"isb"
		: : : "memory");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile(
		"msr BASEPRI, %0;"
		"isb;"
		:  : "r"(key) : "memory");
#elif defined(CONFIG_ARMV7_R)
	if (key) {
		return;
	}
	__asm__ volatile(
		"cpsie i;"
		: : : "memory", "cc");
#else
#error Unknown ARM architecture
#endif
}

static FORCE_INLINE bool arch_irq_unlocked(word_t key)
{
	/* This convention works for both PRIMASK and BASEPRI */
	return key == 0;
}

#ifdef __cplusplus
}
#endif

#endif
#endif
