#ifndef DEFAULT_H_
#define DEFAULT_H_

#define KIP_EXTRA_SIZE 128u
#define KERNEL_WCET_SCALE   1u
#define NUM_PRIORITIES   256u
/* roundrobin thread */
#define MIN_REFILLS   2u 
/* A word size is 32 ~ 2^5*/
#define WORD_SIZE 5u
/* WORD_BITS = 32 */
#define WORD_BITS (1u << WORD_SIZE)
#define NUM_READY_QUEUES (CONFIG_NUM_DOMAINS * NUM_PRIORITIES)
/*each domain sched_prior number ~ 256*/
#define L2_BITMAP_BITS ((NUM_PRIORITIES + WORD_BITS - 1) / WORD_BITS)
#define L1_BITMAP_BITS ((NUM_PRIORITIES + WORD_BITS - 1) / WORD_BITS)
#define MAX_NUM_WORUNITS_PER_PREEMPTION 20u
#define SYSTIMER_MIN_TICKS 10u
#define NUM_SCHED_REFILLS 8

#endif
