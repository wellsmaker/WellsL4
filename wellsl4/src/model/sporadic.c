#include <model/sporadic.h>
#include <kernel/thread.h>
/* functions to manage the circular buffer of
 * sporadic budget replenishments (refills for short).
 *
 * The circular buffer always has at least one item in it.
 *
 * Items are appended at the tail (the back) and
 * removed from the head (the front). Below is
 * an example of a queue with 4 items (h = head, t = tail, x = item, [] = slot)
 * and max size 8.
 *
 * [][h][x][x][t][][][]
 *
 * and another example of a queue with 5 items
 *
 * [x][t][][][][h][x][x]
 *
 * The queue has a minimum size of 1, so it is possible that h == t.
 *
 * The queue is implemented as head + tail rather than head + size as
 * we cannot use the mod operator on all architectures without accessing
 * the fpu or implementing divide.
 */

extern struct thread_sched *current_sched;

/* check a refill queue is ordered correctly */
static bool_t refill_ordered(struct thread_sched *sched)
{
    word_t current = sched->refill_head;
    word_t next = refill_next(sched, sched->refill_head);

    while (current != sched->refill_tail) 
	{
        if (!(REFILL_INDEX(sched, current).refill_time <= REFILL_INDEX(sched, next).refill_time)) 
		{
            return FALSE;
        }
        current = next;
        next = refill_next(sched, current);
    }

    return TRUE;
}

/* compute the sum of a refill queue */
#ifdef CONFIG_DEBUG_BUILD
static ticks_t refill_sum(struct thread_sched *sched)
{
    ticks_t sum = REFILL_HEAD(sched).refill_amount;
    word_t current = sched->refill_head;

    while (current != sched->refill_tail) 
	{
        current = refill_next(sched, current);
        sum += REFILL_INDEX(sched, current).refill_amount;
    }

    return sum;
}

#endif

#ifdef CONFIG_DEBUG_BUILD
#define REFILL_SANITY_START(sched) ticks_t _sum = refill_sum(sched); assert(refill_ordered(sched));
#define REFILL_SANITY_CHECK(sched, budget) \
    do { \
        assert(refill_sum(sched) == budget); assert(refill_ordered(sched)); \
    } while (0)

#define REFILL_SANITY_END(sched) \
    do {\
        REFILL_SANITY_CHECK(sched, _sum);\
    } while (0)
#else
#define REFILL_SANITY_START(sched)
#define REFILL_SANITY_CHECK(sched, budget)
#define REFILL_SANITY_END(sched)
#endif /* CONFIG_DEBUG_BUILD */

/* pop head of refill queue */
static FORCE_INLINE refill_t refill_pop_head(struct thread_sched *sched)
{
    /* queues cannot be smaller than 1 */
    assert(!refill_single(sched));

    refill_t refill = REFILL_HEAD(sched);
    sched->refill_head = refill_next(sched, sched->refill_head);

    /* sanity */
    assert(sched->refill_head < sched->refill_max);
    return refill;
}

/* add item to tail of refill queue */
static FORCE_INLINE void refill_add_tail(struct thread_sched *sched, refill_t refill)
{
    /* cannot add beyond queue size */
    assert(refill_size(sched) < sched->refill_max);

    word_t new_tail = refill_next(sched, sched->refill_tail);
    sched->refill_tail = new_tail;
    REFILL_TAIL(sched) = refill;

    /* sanity */
    assert(new_tail < sched->refill_max);
}

static FORCE_INLINE void maybe_add_empty_tail(struct thread_sched *sched)
{
    if (is_roundrobin(sched)) 
	{
        /* add an empty refill - we track the used up time here */
	    /* round robin is that amount is null, and only has two refills with head and tail*/
        refill_t empty_tail = { .refill_time = current_time, .refill_amount = 0 };
        refill_add_tail(sched, empty_tail);
        assert(refill_size(sched) == MIN_REFILLS);
    }
}

/* thread new ~ refill new ; budget ~ refill ticks number(const);
   at start, a budget split many refills,but budget amout is const,
   so, need a max_refills limit;when we use a refill,if enough,the
   usage need to add refill buffer back('period' means next add
   time point dvalue)*/

void refill_new(struct thread_sched *sched, word_t max_refills, ticks_t budget, ticks_t period)
{
    sched->period = period;
    sched->refill_head = 0;
    sched->refill_tail = 0;
    sched->refill_max = max_refills;
    assert(budget > MIN_BUDGET);
    /* full budget available */
    REFILL_HEAD(sched).refill_amount = budget;
    /* budget can be used from now */
    REFILL_HEAD(sched).refill_time = current_time;
    maybe_add_empty_tail(sched);
    REFILL_SANITY_CHECK(sched, budget);
}

void refill_update(struct thread_sched *sched, ticks_t new_period, ticks_t new_budget, word_t new_max_refills)
{

    /* refill must be initialised in order to be updated - otherwise refill_new should be used */
    assert(sched->refill_max > 0);

    /* this is called on an active thread. We want to preserve the sliding window constraint -
     * so over new_period, new_budget should not be exceeded even temporarily */

    /* move the head refill to the start of the list - it's ok as we're going to truncate the
     * list to size 1 - and this way we can't be in an invalid list position once new_max_refills
     * is updated */
    REFILL_INDEX(sched, 0) = REFILL_HEAD(sched);
	
    sched->refill_head = 0;
    /* truncate refill list to size 1 */
    sched->refill_tail = sched->refill_head;
    /* update max refills */
    sched->refill_max = new_max_refills;
    /* update period */
    sched->period = new_period;

    if (refill_ready(sched))
	{
        REFILL_HEAD(sched).refill_time = current_time;
    }

    if (REFILL_HEAD(sched).refill_amount >= new_budget)
	{
        /* if the heads budget exceeds the new budget just trim it */
        REFILL_HEAD(sched).refill_amount = new_budget;
        maybe_add_empty_tail(sched);
    } 
	else
	{
        /* otherwise schedule the rest for the next period */
        refill_t new = { .refill_amount = (new_budget - REFILL_HEAD(sched).refill_amount),
                         .refill_time = REFILL_HEAD(sched).refill_time + new_period
                       };
        refill_add_tail(sched, new);
    }

    REFILL_SANITY_CHECK(sched, new_budget);
}

static FORCE_INLINE void schedule_used(struct thread_sched *sched, refill_t new)
{
    /* schedule the used amount */
    if (new.refill_amount < MIN_BUDGET && !refill_single(sched)) 
	{
        /* used amount is too small - merge with last and delay */
        REFILL_TAIL(sched).refill_amount += new.refill_amount;
        REFILL_TAIL(sched).refill_time = MAX(new.refill_time, REFILL_TAIL(sched).refill_time);
    } 
	else if (new.refill_time <= REFILL_TAIL(sched).refill_time) 
	{
        REFILL_TAIL(sched).refill_amount += new.refill_amount;
    } 
	else
	{
        refill_add_tail(sched, new);
    }
}

/* refill_budget_check + refill_split_check */
/* The idea is to allocate the amount of time needed to each refills and postpone it to 
the next period until the usage is small enough to be within a refill. For the refill, 
it needs to be divided into usage and remainder. Whether to postpone the usage to the next 
cycle, the remainder does not need to be*/

/* need make a usage split and alloc each refills when out of budge */
/* none-period task */
/* when the task budget is not enough for actual exec */
void refill_budget_check(ticks_t usage, ticks_t capacity)
{
    struct thread_sched *sched = current_sched;
    
    assert(capacity < MIN_BUDGET || refill_full(sched));
    assert(sched->period > 0);
	
    REFILL_SANITY_START(sched);

	/* amount - usage */
	/* task usage split to many parts: each part has a period offset, each add a 
	   part should have a period offset until usage split parts */
    if (capacity == 0) 
	{
		/* The task is divided into multiple subtasks, and each subtask is assigned
		   to the next release time */
        while (REFILL_HEAD(sched).refill_amount <= usage) 
		{
            /* exhaust and schedule replenishment */
            usage -= REFILL_HEAD(sched).refill_amount;
            if (refill_single(sched))
			{
                /* update in place */
                REFILL_HEAD(sched).refill_time += sched->period;
            } 
			else 
			{
				/* update in different place */
                refill_t old_head = refill_pop_head(sched);
                old_head.refill_time = old_head.refill_time + sched->period;
                schedule_used(sched, old_head);
            }
        }

        /* budget overrun */
        if (usage > 0) 
		{
            /* budget reduced when calculating capacity */
            /* due to overrun delay next replenishment */
            REFILL_HEAD(sched).refill_time += usage;
            /* merge front two replenishments if times overlap */
            if (!refill_single(sched) &&
                REFILL_HEAD(sched).refill_time + REFILL_HEAD(sched).refill_amount >=
                REFILL_INDEX(sched, refill_next(sched, sched->refill_head)).refill_time) 
            {

                refill_t refill = refill_pop_head(sched);
                REFILL_HEAD(sched).refill_amount += refill.refill_amount;
                REFILL_HEAD(sched).refill_time = refill.refill_time;
            }
        }
    }

	/* In addition to the usage time exceeding the time budget, it may occur 
	   that the usage time is less than the time budget, but the remaining 
	   time is less than the minimum available time */
    capacity = refill_capacity(sched, usage);
    if (capacity > 0 && refill_ready(sched)) 
	{
		assert(capacity < MIN_BUDGET);
        refill_split_check(usage);
    }

    /* ensure the refill head is sufficient, such that when we wake in awaken,
     * there is enough budget to run */
    while (REFILL_HEAD(sched).refill_amount < MIN_BUDGET || refill_full(sched)) 
	{
        refill_t refill = refill_pop_head(sched);
        REFILL_HEAD(sched).refill_amount += refill.refill_amount;
        /* this loop is guaranteed to terminate as the sum of
         * refill_amount in a refill must be >= MIN_BUDGET */
    }

    REFILL_SANITY_END(sched);
}

/* split ~ 'usage - refill period * n' */
/* none-period task */
/* when the task remnant < MIN_BUDGET or the task preempted */
/* when the task budget is not/yes enough for actual exec */
void refill_split_check(ticks_t usage)
{
    struct thread_sched *sched = current_sched;
    /* invalid to call this on a NULL sched */
    assert(sched != NULL);
    /* something is seriously wrong if this is called and no
     * time has been used */
    assert(usage > 0);
    assert(usage <= REFILL_HEAD(sched).refill_amount);
    assert(sched->period > 0);

    REFILL_SANITY_START(sched);

    /* first deal with the remaining budget of the current replenishment */
    ticks_t remnant = REFILL_HEAD(sched).refill_amount - usage;

    /* set up a new replenishment structure */
    refill_t new = (refill_t) {
        .refill_amount = usage, .refill_time = REFILL_HEAD(sched).refill_time + sched->period
    };

    if (refill_size(sched) == sched->refill_max || remnant < MIN_BUDGET) 
	{
        /* merge remnant with next replenishment - either it's too small
         * or we're out of space */
        if (refill_single(sched)) 
		{
            /* update inplace */
            new.refill_amount += remnant;
            REFILL_HEAD(sched) = new;
        } 
		else
		{
            refill_pop_head(sched);
            REFILL_HEAD(sched).refill_amount += remnant;
            schedule_used(sched, new);
        }
        assert(refill_ordered(sched));
    } 
	else 
	{
        /* leave remnant as reduced replenishment */
        assert(remnant >= MIN_BUDGET);
        /* split the head refill  */
        REFILL_HEAD(sched).refill_amount = remnant;
        schedule_used(sched, new);
    }

    REFILL_SANITY_END(sched);
}

/* Compare the supply time with the current time. If the scheduling context can be used and there 
   are multiple supplies, the supply needs to be merged until the supply head is found to be 
   available or the supply has been merged into one, and the supply time is set to the current time */
/* period or none-period task */
/* when the task start or before start, check the refills whether be a block */
/* if the refills not be a block, just merge until to be a block */
/* It satisfies the premise that the next refresh timestamp is greater than the 
   current time plus the current refill time */
void refill_noblock_check(struct thread_sched *sched)
{
    if (is_roundrobin(sched)) 
	{
        /* nothing to do */
        return;
    }
	
    /* advance earliest activation time to now */
    REFILL_SANITY_START(sched);
    if (refill_ready(sched)) 
	{
        REFILL_HEAD(sched).refill_time = current_time;
        /* merge available replenishments */
        while (!refill_single(sched))
		{
            ticks_t amount = REFILL_HEAD(sched).refill_amount;
            if (REFILL_INDEX(sched, refill_next(sched, sched->refill_head)).refill_time <= current_time + amount) 
			{
                refill_pop_head(sched);
                REFILL_HEAD(sched).refill_amount += amount;
                REFILL_HEAD(sched).refill_time = current_time;
            } 
			else
			{
                break;
            }
        }

        assert(refill_sufficient(sched, 0));
    }
    REFILL_SANITY_END(sched);
}
