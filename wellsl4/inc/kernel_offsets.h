#ifndef OFFSETS_H_
#define OFFSETS_H_

/** gen + */
#include <offsets.h>
#include <arch/kernel_offsets.h>

/* kernel */

/* main */

#define _kernel_offset_to_irq_nested \
	(__cpu_t_irq_nested_OFFSET)

#define _kernel_offset_to_irq_stack \
	(__cpu_t_irq_stack_OFFSET)

#define _kernel_offset_to_current \
	(__cpu_t_current_thread_OFFSET)

#define _kernel_offset_to_idle \
	(__cpu_t_idle_thread_OFFSET)

#define _kernel_offset_to_current_fp \
	(__kernel_t_fp_thread_OFFSET)

#define _kernel_offset_to_ready_q_cache \
	(__kernel_t_ready_thread_OFFSET)

/* end - kernel */

/* threads */

/* main */

#define _thread_offset_to_callee_saved \
	(__ktcb_t_callee_saved_OFFSET)

/* base */

#define _thread_offset_to_thread_state \
	(__thread_base_t_option_OFFSET + __thread_base_t_thread_state_OFFSET)

#define _thread_offset_to_option \
	(__thread_base_t_option_OFFSET + __thread_base_t_option_OFFSET)

#define _thread_offset_to_prio \
	(__thread_base_t_option_OFFSET + __thread_base_t_sched_prior_OFFSET)

#define _thread_offset_to_sched_locked \
	(__thread_base_t_option_OFFSET + __thread_base_t_sched_locked_OFFSET)

#define _thread_offset_to_preempt \
	(__thread_base_t_option_OFFSET + __thread_base_t_preempt_OFFSET)

#define _thread_offset_to_esf \
	(__ktcb_t_arch_OFFSET + __thread_arch_t_esf_OFFSET)

#define _thread_offset_to_stack_start \
	(__ktcb_t_stack_info_OFFSET + __thread_stack_info_t_start_OFFSET)
/* end - threads */

#endif