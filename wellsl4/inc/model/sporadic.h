#ifndef MODEL_SPORADIC_H_
#define MODEL_SPORADIC_H_

#include <types_def.h>
#include <default/default.h>
#include <kernel/time.h>
#include <kernel_object.h>

extern ticks_t current_time;
/* To do an operation in the kernel, the thread must have
 * at least this much budget - see comment on refill_sufficient */
#define MIN_BUDGET_US (2u * get_kernel_wcet_us() * KERNEL_WCET_SCALE)
#define MIN_BUDGET    (2u * get_kernel_wcet_ticks() * KERNEL_WCET_SCALE) /* task switch time(test) */

/* Short hand for accessing refill queue items */
//#define REFILL_INDEX(sc, index) (((refill_t *) ((word_t)(sc) + sizeof(struct thread_sched)))[index])

#define REFILL_INDEX(sc, index) (((refill_t *) (sc->refill_buffer))[index])
#define REFILL_HEAD(sc) REFILL_INDEX((sc), (sc)->refill_head)
#define REFILL_TAIL(sc) REFILL_INDEX((sc), (sc)->refill_tail)

/* @return the current amount of empty slots in the refill buffer */
static inline word_t refill_size(struct thread_sched *sc)
{
    if (sc->refill_head <= sc->refill_tail) {
        return (sc->refill_tail - sc->refill_head + 1u);
    }
    return sc->refill_tail + 1u + (sc->refill_max - sc->refill_head);
}

/*
 * Return TRUE if the head refill is eligible to be used.
 * This indicates if the thread bound to the sc can be placed
 * into the scheduler, otherwise it needs to go into the release queue
 * to wait.
 */
static inline bool_t refill_ready(struct thread_sched *sc)
{
    return REFILL_HEAD(sc).refill_time <= current_time + get_kernel_wcet_ticks();
}

/* return the index of the next item in the refill queue */
static inline word_t refill_next(struct thread_sched *sc, word_t index)
{
	return (index == sc->refill_max - 1u) ? (0) : index + 1u;
}

/* @return true if the circular buffer of refills is current full (all slots in the
 * buffer are currently being used */
static inline bool_t refill_full(struct thread_sched *sc)
{
    return refill_size(sc) == sc->refill_max;
}

/* @return true if the ciruclar buffer only contains 1 used slot */
static inline bool_t refill_single(struct thread_sched *sc)
{
    return sc->refill_head == sc->refill_tail;
}

/* Return the amount of budget this scheduling context
 * has available if usage is charged to it. */
static inline ticks_t refill_capacity(struct thread_sched *sc, ticks_t usage)
{
    if (usage > REFILL_HEAD(sc).refill_amount) {
        return 0;
    }

    return REFILL_HEAD(sc).refill_amount - usage;
}

/*
 * Return true if the head refill has sufficient capacity
 * to enter and exit the kernel after usage is charged to it.
 */
static inline bool_t refill_sufficient(struct thread_sched *sc, ticks_t usage)
{
    return refill_capacity(sc, usage) >= MIN_BUDGET;
}

/* Create a new refill in a non-active sc */
void refill_new(struct thread_sched *sc, word_t max_refills, ticks_t budget, ticks_t period);

/* Update refills in an active sc without violating bandwidth constraints */
void refill_update(struct thread_sched *sc, ticks_t new_period, ticks_t new_budget, word_t new_max_refills);


/* Charge `usage` to the current scheduling context.
 * This function should only be called only when charging `used` will deplete
 * the head refill, resulting in refill_sufficient failing.
 *
 * @param usage the amount of time to charge.
 * @param capacity the value returned by refill_capacity. At most call sites this
 * has already been calculated so pass the value in rather than calculating it again.
 */
void refill_budget_check(ticks_t used, ticks_t capacity);

/*
 * Charge a the current scheduling context `used` amount from its
 * current refill. This will split the refill, leaving whatever is
 * left over at the head of the refill. This is only called when charging
 * `used` will not deplete the head refill.
 */
void refill_split_check(ticks_t used);

/*
 * This is called when a thread is eligible to start running: it
 * iterates through the refills queue and merges any
 * refills that overlap.
 */
void refill_noblock_check(struct thread_sched *sc);

#endif
