/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM specific kernel interface header
 *
 * This header contains the ARM specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arm/cpu.h)
 */

#ifndef ARCH_ARM_AARCH32_ARCH_H_
#define ARCH_ARM_AARCH32_ARCH_H_


#ifndef _ASMLANGUAGE


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
#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define THREAD_STACK_ALIGN_SIZE 8
#else
#define THREAD_STACK_ALIGN_SIZE 4
#endif

/**
 * @brief Declare a minimum MPU right alignment and size
 *
 * This specifies the minimum MPU right alignment/size for the MPU. This
 * will be used to denote the right section of the stack, if it exists.
 *
 * One key note is that this right results in extra bytes being added to
 * the stack. APIs which give the stack ptr and stack size will take this
 * right size into account.
 *
 * Stack is allocated, but initial stack pointer is at the end
 * (highest address).  Stack grows down to the actual allocation
 * address (lowest address).  Stack right, if present, will comprise
 * the lowest MPU_GUARD_ALIGN_AND_SIZE bytes of the stack.
 *
 * As the stack grows down, it will reach the end of the stack when it
 * encounters either the stack right region, or the stack allocation
 * address.
 *
 * ----------------------- <---- Stack allocation address + stack size +
 * |                     |            MPU_GUARD_ALIGN_AND_SIZE
 * |  Some thread data   | <---- Defined when thread is created
 * |        ...          |
 * |---------------------| <---- Actual initial stack ptr
 * |  Initial Stack Ptr  |       aligned to THREAD_STACK_ALIGN_SIZE
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |        ...          |
 * |  Stack Ends         |
 * |---------------------- <---- Stack Buffer Ptr from API
 * |  MPU Guard,         |
 * |     if present      |
 * ----------------------- <---- Stack Allocation address
 *
 */
#if defined(CONFIG_MPU_STACK_GUARD)
#define MPU_GUARD_ALIGN_AND_SIZE CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#else
#define MPU_GUARD_ALIGN_AND_SIZE 0
#endif

/**
 * @brief Declare the minimum alignment for a thread stack
 *
 * Denotes the minimum required alignment of a thread stack.
 *
 * Note:
 * User thread stacks must respect the minimum MPU region
 * alignment requirement.
 */
#if defined(CONFIG_USERSPACE)
#define THREAD_MIN_STACK_ALIGN CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#else
#define THREAD_MIN_STACK_ALIGN THREAD_STACK_ALIGN_SIZE
#endif

/**
 * @brief Declare the MPU right alignment and size for a thread stack
 *        that is using the Floating Point services.
 *
 * For record_threads that are using the Floating Point services under Shared
 * Registers (CONFIG_FP_SHARING=y) mode, the exception stack page_f may
 * contain both the basic stack page_f and the FP caller-saved context,
 * upon exception entry. Therefore, a wide right region is required to
 * guarantee that stack-overflow detection will always be successful.
 */
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING) \
	&& defined(CONFIG_MPU_STACK_GUARD)
#define MPU_GUARD_ALIGN_AND_SIZE_FLOAT CONFIG_MPU_STACK_GUARD_MIN_SIZE_FLOAT
#else
#define MPU_GUARD_ALIGN_AND_SIZE_FLOAT 0
#endif

/**
 * @brief Define alignment of an MPU right
 *
 * Minimum alignment of the start address of an MPU right, depending on
 * whether the MPU architecture enforces a size (and power-of-two) alignment
 * requirement.
 */
#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define MPU_GUARD_ALIGN (MAX(MPU_GUARD_ALIGN_AND_SIZE, \
	MPU_GUARD_ALIGN_AND_SIZE_FLOAT))
#else
#define MPU_GUARD_ALIGN MPU_GUARD_ALIGN_AND_SIZE
#endif

/**
 * @brief Define alignment of a stack buffer
 *
 * This is used for two different things:
 *
 * -# Used in checks for stack size to be a multiple of the stack buffer
 *    alignment
 * -# Used to determine the alignment of a stack buffer
 *
 */
#define STACK_ALIGN MAX(THREAD_MIN_STACK_ALIGN, MPU_GUARD_ALIGN)

/**
 * @brief Define alignment of a privilege stack buffer
 *
 * This is used to determine the required alignment of record_threads'
 * privilege stacks when building with support for user mode.
 *
 * @note
 * The privilege stacks do not need to respect the minimum MPU
 * region alignment requirement (unless this is enforced via
 * the MPU Stack Guard feature).
 */
#if defined(CONFIG_USERSPACE)
#define PRIVILEGE_STACK_ALIGN MAX(THREAD_STACK_ALIGN_SIZE, MPU_GUARD_ALIGN)
#endif

/**
 * @brief Calculate power of two ceiling for a buffer size input
 */
#define POW2_CEIL(x) ((1 << (31 - __builtin_clz(x))) < x ?  \
		1 << (31 - __builtin_clz(x) + 1) : \
		1 << (31 - __builtin_clz(x)))

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
/* Guard is 'carved-out' of the thread stack region, and the supervisor
 * mode stack is allocated elsewhere by gen_priv_stack.py
 */
#define ARCH_THREAD_STACK_RESERVED 0
#else
#define ARCH_THREAD_STACK_RESERVED MPU_GUARD_ALIGN_AND_SIZE
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct thread_stack __noinit \
		__aligned(POW2_CEIL(size)) sym[POW2_CEIL(size)]
#else
#define ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct thread_stack __noinit __aligned(STACK_ALIGN) \
		sym[size + MPU_GUARD_ALIGN_AND_SIZE]
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define ARCH_THREAD_STACK_LEN(size) (POW2_CEIL(size))
#else
#define ARCH_THREAD_STACK_LEN(size) ((size) + MPU_GUARD_ALIGN_AND_SIZE)
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct thread_stack __noinit \
		__aligned(POW2_CEIL(size)) \
		sym[nmemb][ARCH_THREAD_STACK_LEN(size)]
#else
#define ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct thread_stack __noinit \
		__aligned(STACK_ALIGN) \
		sym[nmemb][ARCH_THREAD_STACK_LEN(size)]
#endif

#if defined(CONFIG_USERSPACE) && \
	defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct thread_stack __aligned(POW2_CEIL(size)) \
		sym[POW2_CEIL(size)]
#else
#define ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct thread_stack __aligned(STACK_ALIGN) \
		sym[size + MPU_GUARD_ALIGN_AND_SIZE]
#endif

#define ARCH_THREAD_STACK_SIZEOF(sym) (sizeof(sym) - MPU_GUARD_ALIGN_AND_SIZE)

#define ARCH_THREAD_STACK_BUFFER(sym) \
		((char *)(sym) + MPU_GUARD_ALIGN_AND_SIZE)


struct __packed interrupt_stack {
	char data;
};

/*
enum syscall_alias {
	svc_context_switch_svc 	 = 0,
	svc_irq_offload_svc	= 1,
	svc_runtime_exception_svc	  = 2,
	svc_system_call_svc	= 3,
};
*/

#ifdef __cplusplus
}
#endif

#endif


#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/stack.h>
#include <arch/arm/aarch32/cortex_m/exc.h>
#elif defined(CONFIG_CPU_CORTEX_R)
#include <arch/arm/aarch32/cortex_r/stack.h>
#include <arch/arm/aarch32/cortex_r/exc.h>
#else
#endif

#include <arch/arm/aarch32/syscall.h>
#include <arch/arm/aarch32/thread.h>
#include <kernel_object.h>
#include <arch/arm/aarch32/exc.h>
#include <arch/arm/aarch32/irq.h>
#include <arch/arm/aarch32/error.h>
#include <arch/arm/aarch32/misc.h>
#include <arch/arm/aarch32/nmi.h>
#include <arch/arm/aarch32/arch_inlines.h>
#include <arch/ffs.h>

#ifdef CONFIG_CPU_CORTEX_M
#include <arch/arm/aarch32/cortex_m/cpu.h>
#include <arch/arm/aarch32/cortex_m/memory_map.h>
#include <arch/arm/aarch32/cortex_m/sys_io.h>
#elif defined(CONFIG_CPU_CORTEX_R)
#include <arch/arm/aarch32/cortex_r/cpu.h>
#include <arch/arm/aarch32/cortex_r/sys_io.h>
#endif

#endif
