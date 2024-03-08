#ifndef KERNEL_OBJECT_H_
#define KERNEL_OBJECT_H_


#ifndef _ASMLANGUAGE

#include <types_def.h>
#include <sys/dlist.h>
#include <sys/rb.h>
#include <arch/cpu.h>
#include <default/default.h>

/** object type start */

struct ktcb;
struct message;
struct notifation;
struct partition;
struct thread_page;
struct k_object;
struct interrupt;
struct tcb_queue;
struct kip_info;
struct kip_mem;
struct thread_sched;
struct d_object;
struct kernel;
struct tcb_cause;
struct device;
struct timer_event;


struct cpu {
	/* interrupt count */
	word_t int_nest_count;

	/* interrupt stack pointer base */
	byte_t *int_stack_point;
	
	/* currently scheduled thread */
	struct ktcb *current_thread;
	
	/* one assigned idle thread per CPU */
	struct ktcb *idle_thread;

	/** current syscall frame pointer */
	void *syscall_frame_point;

	/* core this scheduling context */
	byte_t core_id;

/* #if defined(CONFIG_SMP) */
	/* True when _current_thread is allowed to context switch */
	byte_t swap_ok;
/* #endif */
};

typedef struct cpu cpu_t;

struct kernel {
	/* For compatibility with pre-SMP code, union the first CPU
	 * record with the legacy fields so code can continue to use
	 * the "_kernel.XXX" expressions and assembly offsets.
	 */
	struct cpu cpus[CONFIG_MP_NUM_CPUS];

#if defined(CONFIG_SYS_POWER_MANAGEMENT) 
	word_t idle_ticks; /* Number of ticks for kernel idling */
#endif

#if !defined(CONFIG_SMP) 
	/*
	 * ready queue(chosen): can be big, keep after small fields, since some
	 * assembly (e.g. ARC) are limited in the encoding of the offset
	 */
	struct ktcb *ready_thread;
#endif

#if defined(CONFIG_FP_SHARING) 
	/*
	 * A 'current_sse' field does not exist in addition to the 'current_fp'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers into 2 distinct blocks owned by differing record_threads.  In
	 * other words, given that the 'fxnsave/fxrstor' instructions
	 * save/restore both the X87 FPU and XMM registers, it's not possible
	 * for a thread to only "own" the XMM registers.
	 */

	/* thread that owns the FP regs */
	struct ktcb *float_thread;
#endif

#if defined(CONFIG_THREAD_MONITOR)
	/* singly linked list of ALL record_threads */
	struct ktcb *monitor_thread; 
#endif
};

typedef struct kernel kernel_t;

struct tcb_queue {
	struct ktcb *head;
	struct ktcb *tail;
};

typedef struct tcb_queue tcb_queue_t;

typedef struct dschedule {
    dom_t  domain; /* domain number */
    word_t length; /* domain time length */
} dschedule_t;

struct tcb_cause {
	word_t cases;
	struct ktcb *new_thread;
};

typedef struct tcb_cause tcb_cause_t;

/* refill define */
struct refill {
    /* Absolute timestamp from when this refill can be used */
    ticks_t refill_time;
    /* Amount of ticks that can be used from this refill */
    ticks_t refill_amount; 
};

typedef struct refill refill_t;

/* schedule context */
struct thread_sched {
    /* period for this sc -- controls rate at which budget is replenished */
    ticks_t period;

    /* amount of ticks this sc has been scheduled for since Consumed
     * was last called or a timeout exception fired, so the consumed 
     * must be reset when the thread in not-schedule state only use 'yield' */
    ticks_t consumed;

    /* data word that is sent with timeout faults that occur on this scheduling context */

    /* Amount of refills this sc tracks */
    word_t refill_max;
    /* Index of the head of the refill circular buffer */
    word_t refill_head;
    /* Index of the tail of the refill circular buffer */
    word_t refill_tail;
	/* refill circular buffer */
	refill_t refill_buffer[NUM_SCHED_REFILLS];
};

typedef struct thread_sched thread_sched_t;

/* Using typedef deliberately here, this is quite intended to be an opaque
 * type.
 *
 * The purpose of this data type is to clearly distinguish between the
 * declared symbol for a stack (of type struct thread_stack) and the underlying
 * buffer which composes the stack data actually used by the underlying
 * thread; they cannot be used interchangeably as some arches precede the
 * stack buffer region with right areas that trigger a MPU or MMU fault
 * if written to.
 *
 * APIs that want to work with the buffer inside should continue to use
 * char *.
 *
 * Stacks should always be created with THREAD_STACK_DEFINE().
 */
struct __packed thread_stack {
	char data;
};

typedef struct thread_stack thread_stack_t;

/**
 * @typedef monitor_entry_t
 * @brief Thread entry point function type.
 *
 * A thread's entry point function is invoked when the thread starts executing.
 * Up to 3 argument values can be passed to the function.
 *
 * The thread terminates execution permanently if the entry point function
 * returns. The thread is responsible for releasing any shared resources
 * it may own (such as mutexes and dynamically allocated memory), prior to
 * returning.
 *
 * @param p1 First argument.
 * @param p2 Second argument.
 * @param p3 Third argument.
 *
 * @return N/A
 */
#if defined(CONFIG_THREAD_MONITOR)
typedef void (*monitor_entry_t)(void *p1, void *p2, void *p3);
struct monitor_function {
	monitor_entry_t entry_ptr;
	void *para1;
	void *para2;
	void *para3;
};
typedef struct monitor_function monitor_function_t;
#endif

struct thread_state_object {
	uintptr_t obj_ptr;
	word_t obj_state;
};

typedef struct thread_state_object thread_state_object_t;

struct thread_base {
	/* user thread options */
	byte_t option;

	/* thread state */
	thread_state_object_t thread_state;

	/*
	 * scheduler lock count and thread sched_prior
	 *
	 * These two fields control the preemptibility of a thread.
	 *
	 * When the scheduler is locked, sched_locked is decremented, which
	 * means that the scheduler is locked for values from 0xff to 0x01. A
	 * thread is coop if its prio is negative, thus 0x80 to 0xff when
	 * looked at the value as unsigned.
	 *
	 * By putting them end-to-end, this means that a thread is
	 * non-preemptible if the bundled value is greater than or equal to
	 * 0x0080.
	 */
	union {
		struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			byte_t sched_locked;
			byte_t sched_prior;
#else /* LITTLE and PDP */
			byte_t sched_prior;
			byte_t sched_locked;
#endif
		};
		hword_t preempt;
	};

	/* mcp */
	word_t mcp; 

	/* Domain, 1 byte (padded to 1 word) */
	word_t domain;

	/* apply to HM action */
	word_t level;

#ifdef CONFIG_SMP
	/* True for the per-CPU idle */
	byte_t smp_is_idle;

	/* CPU index on which thread was last run */
	byte_t smp_cpu_id;

	/* Recursive count of irq_lock() calls */
	byte_t smp_lock_count;
	
#ifdef CONFIG_SCHED_CPU_MASK
	/* "May run on" bits for each CPU */
	byte_t smp_cpu_mask;
#endif

#endif
	/* data returned by APIs */
	void *swap_data;
};

typedef struct thread_base thread_base_t;

#if defined(CONFIG_THREAD_STACK_INFO)
/* Contains the stack information of a thread */
struct thread_stack_info {
	/* Stack start - Represents the start address of the thread-writable
	 * stack area.
	 */
	uintptr_t start;

	/* Stack Size - Thread writable stack buffer size. Represents
	 * the size of the actual area, starting from the start member,
	 * that should be writable by the thread
	 */
	size_t size;
};

typedef struct thread_stack_info thread_stack_info_t;
#endif



#if defined(CONFIG_USERSPACE)
struct thread_page_table    {

	/** memory domain queue node */
	sys_dnode_t pagetable_node;
	
	/** memory domain of the thread */
	struct thread_page *pagetable_item;
};

typedef struct thread_page_table thread_page_table_t;
#endif

typedef void (*thread_abort_func_t)(void);

/* TBD */
struct utcb {
	word_t mr[8];   /* 8-15 */
	word_t br[8];   /* 0-7 */
};

struct ktcb {
	/** thread base */
	struct thread_base base;
	
	/** defined by the architecture, but all archs need these */
	struct callee_save callee_saved;

	/** defined thread id */
	word_t thread_id;

	/** user-level tcb */
	struct utcb *user;

	/** abort function */
	thread_abort_func_t abort_handle;

	/* notifation */
	struct notifation *notifation_node;

	/* message */
	struct message *message_node;
	
	/* scheduling context that this tcb is running on, if it is NULL the tcb cannot be in the scheduler queues, 1 word */
	struct thread_sched   *sched;

	/* scheduling context that this tcb yielded to , 1words */
	struct ktcb *yield;		

	/* Previous and next pointers for scheduler queues , 1 words */
	struct ktcb *ready_q_next;	
	
	/* Previous and next pointers for scheduler queues , 1 words */
    struct ktcb *ready_q_prev; 	

	/* message node next , 1words */
	struct ktcb  *mesg_q_next;

	/* message node prev , 1words */
	struct ktcb  *mesg_q_prev;
	
#ifdef CONFIG_USERSPACE
	/** Base address of thread stack */
	struct thread_stack *userspace_stack_point;
	
	/** memory domain info of the thread */
	struct thread_page_table userspace_fpage_table;
#endif

#ifdef CONFIG_THREAD_NAME
	/** Thread name */
	char name[CONFIG_THREAD_MAX_NAME_LEN];
#endif

#ifdef CONFIG_ERRNO
	/** per-thread errno variable */
	sword_t errno_var;
#endif
	
#ifdef CONFIG_THREAD_STACK_INFO
	/** Stack Info */
	struct thread_stack_info stack_info;
#endif

#ifdef CONFIG_USE_SWITCH
	/* When using __switch() a few previously arch-specific items
	 * become part of the core OS */
	/** swap_thread() return value */
	sword_t swap_retval;

	/** Context handle returned via arch_switch() */
	void *swap_handler;
#endif

#ifdef CONFIG_THREAD_MONITOR
	/** thread entry and parameters description */
	monitor_function_t monitor_thread_function;

	/** next item in list of all threads */
	struct ktcb *monitor_thread_next;
#endif

	/** arch-specifics: must always be at the end */
	struct thread_arch arch;
};

typedef struct ktcb ktcb_t;

enum thread_status {
	/* Not a real thread - not to set of some actions */
	state_dummy_state = BIT(0),

	/* Thread is waiting on an object - RECEIVE */
	state_recv_blocked_state = BIT(1),

	/* Thread is waiting on an object - SEND */
	state_send_blocked_state = BIT(2),

	/* Thread has not yet started */
	state_restart_state = BIT(3),

	/* Thread has terminated */
	state_dead_state = BIT(4),

	/* Thread is suspended */
	state_suspended_state = BIT(5),

	/* Thread is being aborted (SMP only) */
	state_aborting_state = BIT(6),

	/* Thread is present in the ready queue , can be sched-syscalled */
	state_queued_state = BIT(7),

	/* Thread is blocked by notify */
	state_notify_blocked_state = BIT(8),
};

/* INTERRUPT DES_DEFINE */
struct interrupt {
	struct ktcb  *thread;
	int32_t  number;
	uint16_t action;
	/* uint16_t prior;   */
	/* word_t   handler; */
};
typedef struct interrupt interrupt_t;

typedef enum message_state {
	message_state_idle = 0,
	message_state_send = 1,
	message_state_recv = 2
} message_state_t;

typedef enum notifation_state {
	notifation_state_idle = 0,
	notifation_state_wait,
	notifation_state_active
} notifation_state_t;
	

typedef enum message_label {
	interrupt_control_label =   0x0928,
	
} message_label_t;

struct message {
	message_state_t state; 	/* node state */
	struct tcb_queue  queue; 	/* message node queue*/
};
/* This queue is not storing multiple sending or receiving threads, 
   but storing multiple sending or receiving processes of the same thread */
typedef struct message message_t;

struct notifation {
	notifation_state_t state; 			/* node state */
	struct tcb_queue        queue;   		/* notice wait queue */
	struct ktcb   *bindedtcb; 		/* record self schedcontext */
};

typedef struct notifation notifation_t;

enum obj_tag
{
	obj_any_obj = 0,
    obj_null_obj = 2, /* null is the object of can be deleted */
    obj_untyped_obj = 4, /* untyped is the object of any type object, but you need retype */
    obj_message_obj = 6,
    obj_notification_obj = 8,
    //obj_reply_obj = 10,
    //obj_cnode_obj = 12,
    obj_thread_obj = 14,
    obj_sched_context_obj = 16,
    obj_irq_control_obj = 18,
    obj_irq_handler_obj = 20,
    obj_domain_obj = 22,
    obj_time_obj = 24,
    obj_device_obj = 26,
    obj_pager_obj = 28,
    //obj_small_frame_obj = 1,
    obj_frame_obj = 3,
    //obj_asid_pool_obj = 5,
    //obj_page_table_obj = 7,
    //obj_page_directory_obj = 9,
    //obj_asid_control_obj = 11,

	obj_last_obj
};
typedef enum obj_tag obj_tag_t;


struct k_object
{
	void      *name; /* name equal to object address */
	uintptr_t right; /* object right for option use that the same object between and own */
	byte_t    type; /* object type */
	byte_t    flag; /* object status */
	uintptr_t data; /* object of thread object id, for thread object, the data is own id(0~MAX32)*/
					 /* max owned number is the BIT(0-31) for use */
} __packed __aligned(4);

struct d_object 
{
	struct k_object k_obj;
	sys_dnode_t k_obj_dlnode;
	struct rbnode k_obj_rbnode; /* must be before k_obj_self member */
	byte_t k_obj_self[]; /* The object itself <address = k_object name>, from the this, is the dync alloc kernel object self */
};

typedef word_t(*timer_handler_t)(generptr_t);
typedef struct _dnode timer_node_t;
typedef struct _dnode timer_list_t;

struct timer_event {
	word_t  		dvalue; /*timepoint:_current_thread-before*/
	generptr_t 		data;   /*handler function para ~ tcb*/
	timer_handler_t handler;/*handler function*/
	timer_node_t    index;  /*timer queue index*/
};

typedef struct timer_event timer_event_t;

typedef void (*ktcb_entry_t)(void *p1, void *p2, void *p3);

struct pager_context {
	struct thread_stack *stack_ptr;
	struct utcb *virual_user;
	size_t stack_size;
	ktcb_entry_t entry;
	void *p1;
	void *p2; 
	void *p3; 
	word_t options;
};

typedef struct pager_context pager_context_t;

enum ks_object_type {
	#include <kobj-types-enum.h>
};

typedef struct fastipc_path {
	word_t receive;
	word_t send;
	word_t timeout;
	word_t *anysend;
} fastipc_path_t;

/** object type end */

#endif





#endif
