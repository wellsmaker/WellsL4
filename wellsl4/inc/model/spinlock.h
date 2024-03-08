/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef MODEL_SPINLOCH_
#define MODEL_SPINLOCH_

#ifndef _ASMLANGUAGE

#include <model/atomic.h>
#include <sys/assert.h>
#include <sys/stdbool.h>
#include <arch/cpu.h>
#include <types_def.h>

#define THREAD_CPU_MASK 0x03

/* There's a spinlock validation framework available when asserts are
 * enabled.  It adds a relatively hefty overhead (about 3k or so) to
 * kernel code size, don't use on platforms known to be small.
 */

struct spinlock_key 
{
	word_t key;
};

typedef struct spinlock_key spinlock_key_t;

struct spinlock
{
#if defined(CONFIG_SMP)
	atomic_t locked;
#endif

#if defined(CONFIG_SPIN_VALIDATE) 
	/* Stores the thread that holds the lock with the locking CPU
	 * ID in the bottom two bits.
	 */
	uintptr_t thread_cpu;
#endif

#if defined(CONFIG_CPLUSPLUS) && !defined(CONFIG_SMP) && \
	!defined(CONFIG_SPIN_VALIDATE)
	/* If CONFIG_SMP and CONFIG_SPIN_VALIDATE are both not defined
	 * the spinlock struct will have no members. The result
	 * is that in C sizeof(spinlock) is 0 and in C++ it is 1.
	 *
	 * This size difference causes problems when the spinlock
	 * is embedded into another struct like k_msgq, because C and
	 * C++ will have different ideas on the offsets of the members
	 * that come after the spinlock member.
	 *
	 * To prevent this we add a 1 byte dummy member to spinlock
	 * when the user selects C++ support and spinlock would
	 * otherwise be empty.
	 */
	char dummy;
#endif
};

typedef struct spinlock spinlock_t;

#if defined(CONFIG_SPIN_VALIDATE)  
extern bool_t is_not_spinlock(spinlock_t *l);
extern bool_t is_spinlock_unlock(spinlock_t *l);
extern void set_spinlock(spinlock_t *l);
#endif

static FORCE_INLINE spinlock_key_t lock_spin_lock(spinlock_t *l)
{
	spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = arch_irq_lock();

#if defined(CONFIG_SPIN_VALIDATE) 
	assert_info(is_not_spinlock(l), "Recursive spinlock %p", l);
#else
	ARG_UNUSED(l);
#endif

#if defined(CONFIG_SMP) 
	while (!atomic_cas(&l->locked, 0, 1));
#endif

#if defined(CONFIG_SPIN_VALIDATE) 
	set_spinlock(l);
#endif

	return k;
}

static FORCE_INLINE void unlock_spin_unlock(spinlock_t *l, spinlock_key_t key)
{
#if defined(CONFIG_SPIN_VALIDATE) 
	assert_info(is_spinlock_unlock(l), "Not my spinlock %p", l);
#else
	ARG_UNUSED(l);
#endif

#if defined(CONFIG_SMP) 
	/* Strictly we don't need atomic_clear() here (which is an
	 * exchange operation that returns the old value).  We are always
	 * setting a zero and (because we hold the lock) know the existing
	 * state won't change due to a race.  But some architectures need
	 * a memory barrier when used like this, and we don't have a
	 * WellL4 framework for that.
	 */
	atomic_clear(&l->locked);
#endif

	arch_irq_unlock(key.key);
}

/* Internal function: releases the lock, but leaves local interrupts
 * disabled
 */
static FORCE_INLINE void release_spin_release(spinlock_t *l)
{
#if defined(CONFIG_SPIN_VALIDATE) 
	assert_info(is_spinlock_unlock(l), "Not my spinlock %p", l);
#else
	ARG_UNUSED(l);
#endif

#if defined(CONFIG_SMP) 
	atomic_clear(&l->locked);
#endif
}
#endif
#endif
