#ifndef MODEL_STATEDATA_H_
#define MODEL_STATEDATA_H_

#ifndef _ASMLANGUAGE

#include <types_def.h>
#include <kernel_object.h>
#include <default/default.h>

extern fastipc_path_t fastipc_caller;
extern struct ktcb *record_threads[256];
extern word_t record_thread_count;

extern word_t work_units_completed;

extern struct thread_sched *current_sched;
extern struct ktcb *current_thread;

/* idle thread */
extern struct ktcb privilege_thread;
extern struct ktcb main_thread;
extern struct ktcb idle_thread;

/* Values of 0 and ~0 encode ResumeCurrentThread and ChooseNewThread
 * respectively; other values encode SwitchToThread and must be valid
 * tcb pointers */
extern struct ktcb *scheduler_action;

/* extern struct tcb_cause *induced_causes; */

/* ready queue */
extern struct tcb_queue ready_queues[NUM_READY_QUEUES]; 	 /*index:prior*/

/* thread message node */
/* extern message_t message_queues[]; */

/* release queue */
/* Head of the queue of record_threads waiting for their budget to be replenished */
extern struct ktcb *release_queue;

/* whether we need to reprogram the timer(special for budget time check and refills update) before exiting the kernel */
/* exit the kernel mean 'switch context or thread switch process' */
extern bool_t reprogram;

/* ready queue bitmap by sched_prior */
extern word_t ready_queues_l1_bitmap[CONFIG_NUM_DOMAINS];
extern word_t ready_queues_l2_bitmap[CONFIG_NUM_DOMAINS][L2_BITMAP_BITS];

/* time */
/* the amount of time passed since the kernel time was last updated */
extern ticks_t consume_time;	/*usage time length*/

/* the _current_thread kernel time (recorded on kernel entry) */
extern ticks_t current_time;	/*_current_thread time point*/

/* Domain timeslice remaining */
extern ticks_t current_domain_time;	/*domain can use time*/

/* Currently active domain */
extern dom_t current_domain;

/* An index into domain_schedule for active domain and length. */
extern word_t current_domain_schedule_idx;

/* Default schedule. */
extern dschedule_t domain_schedule[CONFIG_NUM_DOMAINS];
extern word_t domain_length[CONFIG_NUM_DOMAINS];
extern const word_t domain_schedule_length;

extern word_t current_syscall_error_code;
extern word_t current_kernel_status_code;

/* the only struct _kernel instance */
extern struct kernel _kernel;

#if defined(CONFIG_SMP) 
/* True if the current context can be preempted and migrated to
 * another SMP CPU.
 */
#define _current_cpu ({ assert(!smp_cpu_is_vaild()); arch_curr_cpu(); })
#define _current_thread (_current_cpu->current_thread)
#define _idle_thread (_current_cpu->idle_thread)
#define _current_cpu_index (arch_get_curr_cpu_index())

#else
#define _current_cpu (&_kernel.cpus[0])
#define _current_thread (_current_cpu->current_thread)
#define _idle_thread (_current_cpu->idle_thread)
#define _current_cpu_index (0)
#endif

THREAD_STACK_EXTERN(_main_stack);
THREAD_STACK_EXTERN(_idle_stack);
THREAD_STACK_EXTERN(_privilege_stack);
THREAD_STACK_EXTERN(_interrupt_stack);

#ifdef CONFIG_SMP
extern struct ktcb * const idle_thread1;
extern struct ktcb * const idle_thread2;
extern struct ktcb * const idle_thread3;

THREAD_STACK_EXTERN(_idle_stack1);
THREAD_STACK_EXTERN(_interrupt_stack1);
THREAD_STACK_EXTERN(_idle_stack2);
THREAD_STACK_EXTERN(_interrupt_stack2);
THREAD_STACK_EXTERN(_idle_stack3);
THREAD_STACK_EXTERN(_interrupt_stack3);
#endif

#if defined(CONFIG_USERSPACE)
#if defined(CONFIG_MBEDTLS)
extern partition_t mbedtls_partition;
#endif

#if defined(LIBC_PARTITION_EXISTS) 
extern partition_t libc_partition;
#endif

#if defined(CONFIG_STACCANARIES) 
extern volatile uintptr_t __stack_chk_guard;
#endif
#endif

extern s8_t sys_device_level;

#if defined(CONFIG_EXECUTION_BENCHMARKING) 
extern u64_t arch_timing_swap_start;
extern u64_t arch_timing_swap_end;
extern u64_t arch_timing_irq_start;
extern u64_t arch_timing_irq_end;
extern u64_t arch_timing_tick_start;
extern u64_t arch_timing_tick_end;
extern u64_t arch_timing_enter_user_mode_end;
extern u32_t arch_timing_value_swap_end;
extern u64_t arch_timing_value_swap_common;
extern u64_t arch_timing_value_swap_temp;
#endif

#endif
#endif
