#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <kernel_object.h>
#include <arch/irq.h>

#define IRQ_NUM_IS_VAILD(num) (((num) > 0) && ((num) < CONFIG_NUM_IRQS))

enum interrupt_ipc {
	interrupt_message_tag       = 0,
    interrupt_irq_number 		= 1,
    interrupt_thread_id 		= 2,
    interrupt_irq_action 	= 3,
    interrupt_message_number
};

enum interrupt_action {
    interrupt_signal_enable   = 0,
	interrupt_timer_enable,
    interrupt_disable,
    interrupt_free,
    interrupt_action_number
};

enum interrupt_state {
	interrupt_irq_inactive = 0,
	interrupt_irq_signal,
	interrupt_irq_timer,
	interrupt_irq_reserved
};

typedef enum interrupt_state interrupt_state_t;

static FORCE_INLINE void interrupt_array_init(struct interrupt **array)
{
	for(word_t it = 0; it < CONFIG_NUM_IRQS; it ++)	
	{
		array[it] = NULL;
	}
}

void interrupt_request(struct ktcb *send);
void interrupt_respond(struct ktcb *recv);
bool_t do_interrupt_service(int32_t num);
void handle_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif
