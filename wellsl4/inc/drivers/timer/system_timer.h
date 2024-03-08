/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Timer driver API
 *
 *
 * Declare API implemented by system timer driver and used by kernel components.
 */

#ifndef DRIVERS_SYSTEM_TIMER_H_
#define DRIVERS_SYSTEM_TIMER_H_

#include <sys/stdbool.h>
#include <types_def.h>

#ifdef __cplusplus
extern "C" {
#endif

/* void clock_isr(void); */
u32_t clock_cycle_get_32(void);
void clock_disable(void);

/**
 * @brief Initialize system clock driver
 *
 * The system clock is a WellL4 device created globally.  This is its
 * initialization callback.  It is a weak symbol that will be
 * implemented as a noop if undefined in the clock driver.
 */
void clock_init(void);

/**
 * @brief Set system clock timeout
 *
 * Informs the system clock driver that the next needed call to
 * update_timelist() will not be until the specified number of ticks
 * from the the current time have elapsed.  Note that spurious calls
 * to update_timelist() are allowed (i.e. it's legal to announce
 * every tick and implement this function as a noop), the requirement
 * is that one tick announcement should occur within one tick BEFORE
 * the specified expiration (that is, passing ticks==1 means "announce
 * the next tick", this convention was chosen to match legacy usage).
 * Similarly a ticks value of zero (or even negative) is legal and
 * treated identically: it simply indicates the kernel would like the
 * next tick announcement as soon as possible.
 *
 * Note that ticks can also be passed the special value FOREVER,
 * indicating that no future timer interrupts are expected or required
 * and that the system is permitted to enter an indefinite sleep even
 * if this could cause rollover of the internal counter (i.e. the
 * system uptime counter is allowed to be wrong, see
 * k_enable_sys_clock_always_on()).
 *
 * Note also that it is conventional for the kernel to pass INT_MAX
 * for ticks if it wants to preserve the uptime tick count but doesn't
 * have a specific event to await.  The intent here is that the driver
 * will schedule any needed timeout as far into the future as
 * possible.  For the specific case of INT_MAX, the next call to
 * update_timelist() may occur at any point in the future, not just
 * at INT_MAX ticks.  But the correspondence between the announced
 * ticks and real-world time must be correct.
 *
 * A final note about SMP: note that the call to clock_set_timeout()
 * is made on any CPU, and reflects the next timeout desired globally.
 * The resulting calls(s) to update_timelist() must be properly
 * serialized by the driver such that a given tick is announced
 * exactly once across the system.  The kernel does not (cannot,
 * really) attempt to serialize things by "assigning" timeouts to
 * specific CPUs.
 *
 * @param ticks Timeout in tick units
 * @param idle Hint to the driver that the system is about to enter
 *        the idle state immediately after setting the timeout
 */
void clock_set_timeout(s32_t ticks, bool idle);

/**
 * @brief Timer idle exit notification
 *
 * This notifies the timer driver that the system is exiting the idle
 * and allows it to do whatever bookkeeping is needed to restore timer
 * operation and compute elapsed ticks.
 *
 * @note Legacy timer drivers also use this opportunity to call back
 * into update_timelist() to notify the kernel of expired ticks.
 * This is allowed for compatibility, but not recommended.  The kernel
 * will figure that out on its own.
 */
void clock_idle_exit(void);

/**
 * @brief Announce time progress to the kernel
 *
 * Informs the kernel that the specified number of ticks have elapsed
 * since the last call to update_timelist() (or system startup for
 * the first call).  The timer driver is expected to delivery these
 * announcements as close as practical (subject to hardware and
 * latency limitations) to tick boundaries.
 *
 * @param ticks Elapsed time, in ticks
 */
extern void update_timelist(s32_t ticks);

/**
 * @brief Ticks elapsed since last update_timelist() call
 *
 * Queries the clock driver for the current time elapsed since the
 * last call to update_timelist() was made.  The kernel will call
 * this with appropriate locking, the driver needs only provide an
 * instantaneous answer.
 */
u32_t clock_elapsed(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SYSTEM_TIMER_H_ */
