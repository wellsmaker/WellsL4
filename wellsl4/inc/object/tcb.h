#ifndef OBJECT_TCB_H_
#define OBJECT_TCB_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <api/errno.h>
#include <sys/stdbool.h>
#include <toolchain.h>
#include <types_def.h>
#include <state/statedata.h>
#include <arch/irq.h>
#include <model/spinlock.h>
#include <kernel_object.h>
#include <api/errno.h>

#if defined(CONFIG_STACK_SENTINEL) 
/* Magic value in lowest bytes of the stack */
#define STACSENTINEL 0xF0F0F0F0
#endif

enum special_thread_id {
	id_idle_id = 1, 
	id_main_id = 2,
	id_privilege_id = 3,
	id_scheduler_id = 4,
	id_spacer_id = 5,
	id_irq_request_id = 6,
	id_irq_ack_id = 7,
	id_end_special_id
	/* user thread id start at 256 */
};

#define GLOBALID_NILTHREAD    0x00000000
#define GLOBALID_ANYTHREAD	  0xFFFFFFFF
#define GLOBALID_TO_TID(id)	(id >> 14)
#define TID_TO_GLOBALID(id)	(id << 14)

enum thread_option {
	/**
	 * @brief system thread that must not abort
	 * @req K-THREAD-000
	 * */
	option_essential_option = BIT(0),
	
#if defined(CONFIG_FP_SHARING)
	/**
	 * @brief thread uses floating point registers
	 */
	option_fp_option = BIT(1),
#endif
	
	/**
	 * @brief user mode thread
	 *
	 * This thread has dropped from supervisor mode to user mode and consequently
	 * has additional restrictions
	 */
	option_user_option = BIT(2),
};

/*This indicates which stack pointer corresponds to the stack
page_f and what operation mode the was processor was in before 
the entry occurred*/
/*
THREAD:execute application software,the process enters THREAD 
when it comes out of reset.CONTROL controls software is priv or
un-priv.
HANDLE:handle exceptions,if finished exception processing,returns
to THREAD.exception always is priv.

priv:stack point is MSP
un-priv:stack point is PSP
*/
enum thread_excret {
	EXC_RESET_VALUE  = 0xFFFFFFFF,
	HANDLE_NFP_MSP = 0xFFFFFFF1,
	THREAD_NFP_MSP = 0xFFFFFFF9,
	THREAD_NFP_PSP = 0xFFFFFFFD,
	HANDLE_FP_MSP  = 0xFFFFFFE1,
	THREAD_FP_MSP  = 0xFFFFFFE9,
	THREAD_FP_PSP  = 0xFFFFFFED
};

typedef enum thread_excret thread_excret_t;

#define CONTROL_PRIV_MASK 0x0001 /*0-priv,1-unpriv*/
#define CONTROL_SP_MASK   0x0002 /*0-MSP,1-PSP*/
#define CONTROL_FP_MASK   0x0004 /*0-NFP,1-FP*/

/* syscall control parameter */
#define CONTROL_BIT_d (1u << 9)
#define CONTROL_BIT_h (1u << 8)
#define CONTROL_BIT_p (1u << 7)
#define CONTROL_BIT_u (1u << 6)
#define CONTROL_BIT_f (1u << 5)
#define CONTROL_BIT_i (1u << 4)
#define CONTROL_BIT_s (1u << 3)
#define CONTROL_BIT_S (1u << 2)
#define CONTROL_BIT_R (1u << 1)
#define CONTROL_BIT_H (1u << 0)
#define CONTROL_BIT_MASK(n,mask) (n & mask)
#define UNDEFINE_VALUE 0xFFFFFFFF

typedef void (*thread_user_cb_t)(const struct ktcb *thread, void *user_data);

static FORCE_INLINE bool_t is_user_vaild(word_t id)
{
	return(GLOBALID_TO_TID(id) >= id_end_special_id);
}

extern void thread_abort(struct ktcb *thread);
struct ktcb *get_thread(word_t id);

#if defined(CONFIG_THREAD_MONITOR)
void thread_foreach(thread_user_cb_t user_cb, void *user_data);
void thread_foreach_exit(struct ktcb *thread);
#endif

bool_t is_thread_in_isr(void);

#if defined(CONFIG_THREAD_NAME)
void thread_set_name(struct ktcb *thread, const string value);
const string thread_get_name(struct ktcb *thread);
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS)
void thread_busy_wait(word_t usec_to_wait);
#endif

#if defined(CONFIG_STACK_SENTINEL)
void check_stack_sentinel(void);
#endif

#if !defined(CONFIG_STACK_POINTER_RANDOM)
size_t adjust_stack_size(size_t stack_size);
#endif

#if defined(CONFIG_SPIN_VALIDATE)
bool_t is_not_spinlock(spinlock_t *spin_lock);
bool_t is_spinlock_unlock(spinlock_t *spin_lock);
void set_spinlock(spinlock_t *spin_lock);
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
void disable_float_mode(struct ktcb *thread);
#endif

#if defined(CONFIG_IRQ_OFFLOAD) 
void thread_irq_offload(irq_offload_routine_t routine, void *parameter);
#endif

struct ktcb *thread_init(struct ktcb *thread, s8_t *stack_start, size_t stackSize, word_t options);
void thread_deinit(struct ktcb *thread);
FUNC_NORETURN void thread_user_mode_enter(ktcb_entry_t entry,
			void *p1, void *p2, void *p3);

struct ktcb *thread_alloc(word_t id);
void thread_free(struct ktcb *thread);
void set_schedule_context(struct ktcb *thread, ticks_t budget, ticks_t period, 
									word_t max_refills);
void set_thread_state(struct ktcb *thread, word_t ts);
void set_thread_state_object(struct ktcb *thread, uintptr_t object);
uintptr_t get_thread_state_object(struct ktcb *thread);
word_t get_thread_state(struct ktcb *thread, word_t ts);
word_t get_thread_object_state(struct ktcb *thread);
void set_domain(struct ktcb *thread, dom_t dom);
void set_thread_action(struct ktcb *thread, word_t action);
word_t get_thread_action(struct ktcb *thread);
void set_new_thread(struct ktcb *new_thread, struct thread_stack *stack, 
						size_t stack_size, ktcb_entry_t entry,
		       			void *p1, void *p2, void *p3, word_t options);
void prepare_start_thread(struct ktcb *thread);
struct ktcb *thread_create(struct ktcb *new_thread, struct thread_stack *stack, 
						size_t stack_size, ktcb_entry_t entry, 
						void *p1, void *p2, void *p3, word_t options);
void thread_destroy(struct ktcb *thread);
void schedule_tcb(struct ktcb *thread);
void thread_activate(void);
void thread_resume(struct ktcb *thread);
void thread_suspend(struct ktcb *thread);
void thread_restart(struct ktcb *thread);
void thread_single_abort(struct ktcb *thread);
void thread_yield(struct ktcb *thread);
void thread_donate(struct ktcb *src, struct ktcb *dest);

/*
__syscall exception_t exchange_registers(
	word_t  dest_thread_id,
	word_t  control,
	word_t  *sp,
	word_t  *ip,
	word_t  *flag);
__syscall exception_t thread_control( 
	word_t dest_thread_id,
	word_t dest_space_id,
	word_t dest_schedule,
	word_t dest_pager_id);
__syscall exception_t switch_thread(word_t dest_thread_id);
__syscall exception_t schedule_control( 
	word_t dest_thread_id,
	word_t dest_time, 
	word_t dest_process, 
	word_t dest_prior, 
	word_t dest_domain);
*/

#endif
#ifdef __cplusplus
}
#endif

#endif
