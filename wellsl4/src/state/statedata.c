#include <state/statedata.h>
#include <linker/sections.h>
#include <user/anode.h>

fastipc_path_t fastipc_caller;

struct ktcb *record_threads[256];
word_t record_thread_count;

word_t work_units_completed;

struct thread_sched *current_sched;
struct ktcb *current_thread;

/* the only struct __kernel instance */
struct kernel _kernel;

/* Values of 0 and ~0 encode ResumeCurrentThread and ChooseNewThread
 * respectively; other values encode SwitchToThread and must be valid
 * tcb pointers */
struct ktcb *scheduler_action;

/* struct tcb_cause *induced_causes; */

/* ready queue */
struct tcb_queue ready_queues[NUM_READY_QUEUES]; 	 /*index:prior*/

/* thread message node */
/* message_t message_queues[CONFIG_MAX_MESSAGE_NODES_ALL]; */

/* release queue */
/* Head of the queue of record_threads waiting for their budget to be replenished */
struct ktcb *release_queue;

/* whether we need to reprogram the timer(special for budget time check and refills update) before exiting the kernel */
/* exit the kernel mean 'switch context or thread switch process' */
bool_t reprogram;

/* ready queue bitmap by sched_prior */
word_t ready_queues_l1_bitmap[CONFIG_NUM_DOMAINS];
word_t ready_queues_l2_bitmap[CONFIG_NUM_DOMAINS][L2_BITMAP_BITS];

/* time */
/* the amount of time passed since the kernel time was last updated */
ticks_t consume_time;	/*usage time length*/

/* the _current_thread kernel time (recorded on kernel entry) */
ticks_t current_time;	/*_current_thread time point*/

/* Domain timeslice remaining */
ticks_t current_domain_time;	/*domain can use time*/

/* Currently active domain */
dom_t current_domain;

/* An index into domain_schedule for active domain and length. */
word_t current_domain_schedule_idx;

/* Default schedule. */
dschedule_t domain_schedule[CONFIG_NUM_DOMAINS];
const word_t domain_schedule_length = sizeof(domain_schedule) / sizeof(dschedule_t);

word_t current_syscall_error_code;
word_t current_kernel_status_code;

/* boot time measurement items */
#if defined(CONFIG_BOOT_TIME_MEASUREMENT) 
word_t __noinit timestamp_main;  /* timestamp when main task starts */
word_t __noinit timestamp_idle;  /* timestamp when CPU goes idle */
#endif

/*
 * Symbol referenced by GCC compiler generated code for canary value.
 * The canary value gets initialized in cstart().
 */
#if defined(CONFIG_USERSPACE) 
#if defined(LIBC_PARTITION_EXISTS) 
APPMEM_PARTITION_DEFINE(libc_partition);
APP_DMEM(libc_partition) uintptr_t __stack_chk_guard;
#endif
#if defined(CONFIG_MBEDTLS) 
APPMEM_PARTITION_DEFINE(mbedtls_partition);
#endif
#else
__noinit uintptr_t __stack_chk_guard;
#endif

s8_t sys_device_level;


#if defined(CONFIG_EXECUTION_BENCHMARKING) 
u64_t  arch_timing_swap_start;
u64_t  arch_timing_swap_end;
u64_t  arch_timing_irq_start;
u64_t  arch_timing_irq_end;
u64_t  arch_timing_tick_start;
u64_t  arch_timing_tick_end;
u64_t  arch_timing_enter_user_mode_end;
u32_t arch_timing_value_swap_end;
u64_t arch_timing_value_swap_common;
u64_t arch_timing_value_swap_temp;
#endif

