#include <object/tcb.h>
#include <object/ipc.h>
#include <object/interrupt.h>
#include <kernel/time.h>
#include <sys/assert.h>
#include <state/statedata.h>
#include <object/objecttype.h>
#include <model/spinlock.h>
#include <kernel/thread.h>
#include <device.h>
#include <api/errno.h>

static spinlock_t interrupt_lock;       /* k_obj struct data */

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

static struct interrupt *interrupt_array[CONFIG_NUM_IRQS] = {NULL};
static interrupt_state_t interrupt_state_array[CONFIG_NUM_IRQS] = {interrupt_irq_inactive};

static bool_t is_interrupt_active(int32_t num)
{
	return interrupt_state_array[num] != interrupt_irq_inactive;
}

static void set_interrupt_active(interrupt_state_t state, int32_t num)
{
	interrupt_state_array[num] = state;
}

static struct interrupt *interrupt_create(int32_t num)
{
	LOCKED(&interrupt_lock)
	{
		if (IRQ_NUM_IS_VAILD(num))
		{
			struct interrupt *irq = 
				(struct interrupt *)d_object_alloc(obj_irq_control_obj, 0);
			
			irq->action = (uint16_t) interrupt_action_number;
			irq->number = num;
			irq->thread = NULL;
			
			if(!interrupt_array[num]) 
				interrupt_array[num] = irq;
			
			return(irq);
		}
	}

	return(NULL);
}

static bool_t interrupt_destroy(int32_t num)
{
	LOCKED(&interrupt_lock)
	{
		if (IRQ_NUM_IS_VAILD(num))
		{
			struct interrupt *irq = interrupt_array[num];
			
			if (irq)
			{
				interrupt_array[num] = NULL;
				d_object_free(irq);
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

void interrupt_request(struct ktcb *s_thread)
{
	message_tag_t tag = {
		.raw = load_message_registers(s_thread, interrupt_message_tag) 
	};

	if (message_get_tag_label(tag) == interrupt_control_label)
	{
		int32_t num = (int32_t) load_message_registers(s_thread, interrupt_irq_number);
		word_t id = (word_t)load_message_registers(s_thread, interrupt_thread_id);
		uint16_t act = (uint16_t)load_message_registers(s_thread, interrupt_irq_action);

		assert(IRQ_NUM_IS_VAILD(num) && is_user_vaild(id));
		assert(act < interrupt_action_number);

		irq_disable(num);
		
		struct interrupt *irq = interrupt_create(num);
		
		if (irq)
		{
			if (!is_interrupt_active(num))
			{
				irq->number = num;
				irq->thread = get_thread(id);
				irq->action = act;
			}
			else
			{
				user_error("Rejecting request for IRQ %u. Already active\n", num);
			}
		}

	}
}

void interrupt_respond(struct ktcb *r_thread)
{
	assert(r_thread);
	word_t lock;
	
	for (int32_t num = 0; num < CONFIG_NUM_IRQS; num++)
	{
		struct interrupt *irq = interrupt_array[num];
		
		if (!irq || irq->thread != r_thread) 
			continue;

		switch (irq->action)
		{
			case interrupt_signal_enable:
			case interrupt_timer_enable:
				lock = irq_lock(); 
				if (irq->action == interrupt_signal_enable)
				{
					set_interrupt_active(interrupt_irq_signal, num);
				}
				else 
				{
					set_interrupt_active(interrupt_irq_timer, num);
				}
				irq_unlock(lock);
				irq_enable(num);
				break;
			case interrupt_disable:
			case interrupt_free:
				lock = irq_lock(); 
			    set_interrupt_active(interrupt_irq_inactive, num);
				if (irq->action == interrupt_free)
				{
					interrupt_destroy(num);
				}
				irq_unlock(lock);
			    irq_disable(num);
				break;
			default:
				lock = irq_lock();
				set_interrupt_active(interrupt_irq_reserved, num);
				irq_unlock(lock);
				irq_disable(num);
				user_error("INTERRUPT CONTROL OBJECT:llegal operation\n");
				break;
		}
	}
}

static void interrupt_signal(int32_t num)
{
	struct interrupt *irq  = interrupt_array[num];
	struct ktcb  *thread  = irq->thread;
	word_t lock;
	
	assert(irq && thread);

	lock = irq_lock();
	if (thread)
	{
		if (get_thread_state(thread, state_recv_blocked_state | 
			state_send_blocked_state) != 0)
		{
			cancel_ipc(thread);
		}

		/** Interrupt handle sends signal and interrupt thread receives signal */
		send_signal(thread->notifation_node);
		
		set_thread_state(thread, state_queued_state);
		
		if (is_schedulable(thread))
		{
			possible_switchto(thread);
		}
	}
	irq_unlock(lock);
}

#if defined(CONFIG_KERNEL_TIMER_INT) 
static bool_t interrupt_time(int32_t num)
{
	if(CONFIG_KERNEL_TIMER_INT == num)
	{
		extern void clock_isr(void);
		clock_isr();
		/* ack deadline event and other event at timer_handler */
		reprogram = TRUE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
#else
static bool_t interrupt_time(int32_t num)
{
	return FALSE;
}
#endif

bool_t do_interrupt_service(int32_t num)
{
	if (!IRQ_NUM_IS_VAILD(num))
	{
		user_error("Received IRQ %d, which is above the platforms maxIRQ of %d\n", 
			num, CONFIG_NUM_IRQS);
		/* clear interrupt */
		irq_disable(num);
		return FALSE;
	}
	
	if (interrupt_state_array[num] == interrupt_irq_reserved)
	{
		user_error("Received unhandled reserved IRQ: %d\n", num);
		return FALSE;
	}

	if (interrupt_state_array[num] == interrupt_irq_inactive)
	{
		user_error("Received disabled IRQ: %d\n", num);
		return FALSE;
	}

	update_timestamp(false); 

	if (interrupt_state_array[num] == interrupt_irq_timer)
	{
		if (interrupt_time(num) == TRUE)
		{
			/* clear interrupt */
			irq_disable(num);
			return TRUE;
		}
	}

	if (check_budget()) 
	{ 
		switch (interrupt_state_array[num])
		{
			case interrupt_irq_signal:
			case interrupt_irq_timer:
				/* send a signal of a interrupt */
				interrupt_signal(num);
				break;
			default:
				break;
		}
		/* clear interrupt */
		irq_disable(num);
	} 

	return TRUE;
}

ISR_DIRECT_DECLARE(handle_interrupt)
{
	word_t irq_num = irq_is_pending();
	bool_t is_resched = FALSE;
	
	if (irq_num != CONFIG_NUM_IRQS)
	{
		is_resched = do_interrupt_service(irq_num);
		/* re-schedule is vaild */
		return is_resched;
	}

	return is_resched;
}

static s32_t init_interrupt_object_module(struct device *dev)
{
	ARG_UNUSED(dev);
	interrupt_array_init(interrupt_array);
	return 0;
}

SYS_INIT(init_interrupt_object_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
