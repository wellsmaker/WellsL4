#include <object/ipc.h>
#include <object/tcb.h>
#include <object/kip.h>
#include <object/interrupt.h>
#include <kernel/time.h>
#include <model/sporadic.h>
#include <sys/assert.h>
#include <model/spinlock.h>
#include <state/statedata.h>
#include <kernel/thread.h>
#include <api/errno.h>
#include <arch/registers.h>
#include <object/anode.h>
#include <object/objecttype.h>
#include <api/syscall.h>
#include <kernel/privilege.h>

static spinlock_t ipc_lock;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

/* IPC can not only transfer a small amount of messages through FAST, 
   but also transfer process status; this effect is beneficial to both 
   the sender and the receiver */
static exception_t message_exchange(struct ktcb *s_thread, struct ktcb *r_thread)
{
	word_t untyped_item_index = 1;
	word_t gmsc_item_index = 0;
	word_t gmsc_item_word = 0;

	assert(s_thread != NULL && r_thread != NULL);
	
	message_tag_t send_msg_tag = { .raw = load_message_registers(s_thread, 0) };
	word_t untyped_item_last = message_get_tag_u(send_msg_tag) + 1;
	word_t gmsc_item_last   = message_get_tag_t(send_msg_tag) + untyped_item_last;
	
	/* overflow */
	word_t message_max = gmsc_item_last;
	
	if (message_max > MESSAGE_REGISTER_NUM)
	{
		current_kernel_status_code = IPC_MSG_OVERFLOW;

		set_thread_state(s_thread, state_queued_state);
		set_thread_state(r_thread, state_queued_state);
		
		return EXCEPTION_FAULT;
	}

	store_message_registers(r_thread, 0, message_get_tag(send_msg_tag));
	
	/* untyped words */
	/* Fast IPC that the length of message is 8 when the thread switch to myself */
	/* Slow IPC that the length of message is greater than 8 */
	for (untyped_item_index = 1; untyped_item_index < untyped_item_last;
		untyped_item_last++)
	{
		store_message_registers(r_thread, untyped_item_index, 
			load_message_registers(s_thread, untyped_item_index));
	}
		
	/* typed words */
	for (gmsc_item_index = untyped_item_index; gmsc_item_index < gmsc_item_last;
		gmsc_item_index++)
	{
		message_gmsc_items_t gmsc_item_cur;
 
		if (gmsc_item_word == 0)
		{
			TYPED_ITEM(gmsc_item_cur)[gmsc_item_word] = 
				load_message_registers(s_thread, gmsc_item_index);
			gmsc_item_word ++;
		}
		else
		{
			TYPED_ITEM(gmsc_item_cur)[gmsc_item_word] = 
				load_message_registers(s_thread, gmsc_item_index);
			gmsc_item_word --;
		}

		store_message_registers(r_thread, gmsc_item_index, 
			TYPED_ITEM(gmsc_item_cur)[gmsc_item_word]);

		if (gmsc_item_word % 2) 
			continue;

		if (message_get_encode(gmsc_item_cur) == MAP_ITEM) 
		{
			do_map_page(r_thread, message_get_base(gmsc_item_cur), 
				message_get_page(gmsc_item_cur), message_get_right(gmsc_item_cur)); 
			/* map process means 'temporary share', but the page always to be send, not receive */
		}
			
		if (message_get_encode(gmsc_item_cur) == GRANT_ITEM) 
		{
			do_guard_page(s_thread, r_thread, message_get_base(gmsc_item_cur), 
				message_get_page(gmsc_item_cur), message_get_right(gmsc_item_cur)); 
			/* grant process means 'permanent give', the page always to be receive, not to be send */
		}	
			
		if (message_get_encode(gmsc_item_cur) == STRING_ITEM)
		{
			do_map_string(r_thread, message_get_length(gmsc_item_cur), 
				message_get_address(gmsc_item_cur));
		}

		if (message_get_encode(gmsc_item_cur) == CTRLXFER_ITEM)
		{
			do_control_registers(r_thread, message_get_id(gmsc_item_cur),
				message_get_mask(gmsc_item_cur), message_get_reg(gmsc_item_cur));
		}
	}
		
	return EXCEPTION_NONE;
}

struct message *message_node_alloc(struct ktcb *thread)
{
	struct message *node = (struct message *)d_object_alloc(obj_message_obj, 0);

	if (!node)
	{
		return NULL;
	}

	thread->message_node = node;
	return node;
}

void message_node_free(struct ktcb *thread)
{
	d_object_free(thread->message_node);
	thread->message_node = NULL;
}

struct notifation *notifation_node_alloc(struct ktcb *thread)
{
	struct notifation *node = (struct notifation *)d_object_alloc(obj_notification_obj, 0);

	if (!node)
	{
		return NULL;
	}

	thread->notifation_node = node;
	return node;
}

void notifation_node_free(struct ktcb *thread)
{
	d_object_free(thread->notifation_node);
	thread->notifation_node = NULL;
}

/* thread : send, node : receive */
void send_ipc(struct ktcb *thread, bool_t blocking, bool_t candonate, message_t *node)
{
	struct tcb_queue queue;
	struct ktcb *r_thread;

	assert(thread != NULL);
	assert(node != NULL);

	LOCKED(&ipc_lock)
	{
		switch (node->state)
		{
			case message_state_idle:
			case message_state_send:
				if(blocking)
				{
					set_thread_state(thread, state_send_blocked_state);
					set_thread_state_object(thread, (uintptr_t)node);
					
					schedule_tcb(thread);
					
					queue = node->queue;
					queue = message_append(thread, queue);
					
					node->state = message_state_send;
					node->queue = queue;
				}
				break;
			case message_state_recv:
	
				queue = node->queue;
				r_thread  = queue.head;
	
				assert(r_thread);
	
				queue = message_dequeue(r_thread, queue);
	
				if(!queue.head)
				{
					node->state = message_state_idle;
				}
	
				node->queue = queue;
				
				/* message transfer */
				if (message_exchange(thread, r_thread) == EXCEPTION_NONE)
				{
					if (candonate && r_thread->sched == NULL) 
					{
						thread_donate(thread, r_thread);
					}
						
					assert(r_thread->sched == NULL || refill_sufficient(r_thread->sched, 0));
					assert(r_thread->sched == NULL || refill_ready(r_thread->sched));
					
					set_thread_state(r_thread, state_queued_state);
					possible_switchto(r_thread);
				}
				break;
			default:
				break;
		}
	}
}


/* thread : recevie, node : receive */
void receive_ipc(struct ktcb *thread, bool_t blocking, message_t *node)
{
	struct tcb_queue queue;
	struct ktcb *s_thread;

	assert(thread != NULL);
	assert(node != NULL);

	LOCKED(&ipc_lock)
	{
		notifation_t *not_node = thread->notifation_node;
		
		if (not_node && not_node->state == notifation_state_active)
		{
			complete_signal(thread, not_node);
		}
		else
		{
			switch (node->state)
			{
				case message_state_idle:
				case message_state_recv:
					if(blocking)
					{
						set_thread_state(thread, state_recv_blocked_state);
						set_thread_state_object(thread, (uintptr_t)node);
						schedule_tcb(thread);
	
						queue = node->queue;
						queue = message_append(thread, queue);
	
						node->state = message_state_recv;
						node->queue = queue;
					}
					break;
				case message_state_send:
	
					queue = node->queue;
					s_thread = queue.head;
	
					assert(s_thread);
	
					queue = message_dequeue(s_thread, queue);
	
					node->queue = queue;
					
					if(!queue.head)
					{
						node->state = message_state_idle;
					}
	
					/* message transfer */
					if (message_exchange(s_thread, thread) == EXCEPTION_NONE)
					{
						set_thread_state(s_thread, state_queued_state);
						possible_switchto(s_thread);
						
						assert(s_thread->sched == NULL || refill_sufficient(s_thread->sched, 0));
					}
					break;
				default:
					break;
			}
		}
	}
}

void cancel_ipc(struct ktcb *thread)
{
	assert(thread != NULL);

	word_t state = get_thread_object_state(thread);

	LOCKED(&ipc_lock)
	{
		switch (state)
		{
			message_t *node;
			struct tcb_queue queue;
			
			case state_send_blocked_state:
			case state_recv_blocked_state:
				node = (message_t *)get_thread_state_object(thread);
				queue = node->queue;
				
				assert(node->state != message_state_idle);
				
				queue = message_dequeue(thread, queue);
				node->queue = queue;
				if(!queue.head)
				{
					node->state = message_state_idle;
				}
				
				set_thread_state(thread, state_restart_state);
				break;
			
			case state_notify_blocked_state:
				cancel_signal(thread, thread->notifation_node);
				break;
			default:
				break;
		}
	}
}

void reorder_message_node(struct ktcb *thread)
{
	assert(thread != NULL);

	message_t *node;
	struct tcb_queue queue;

	LOCKED(&ipc_lock)
	{
		node = (message_t *)get_thread_state_object(thread);
		queue = node->queue;
		
		queue = message_dequeue(thread, queue);
		queue = message_append(thread, queue);
		node->queue = queue;
	}
}

/* make the thread context resume, and make the thread can be scheduled */
static void possible_donate_context(struct ktcb *thread, notifation_t *node)
{
	if (thread->sched == NULL)
	{
		struct ktcb *bind = node->bindedtcb;
		if (bind != NULL)
		{
			thread_donate(bind,thread);
			refill_noblock_check(thread->sched);
			thread_resume(thread);
		}
	}
}

/* reset context , means the thread canot be scheduled, so only at signal vaild thread, the thread can be scheduled */
static void possible_reset_context(struct ktcb *thread, notifation_t *node)
{
	struct ktcb *bind = node->bindedtcb;
	if (bind->sched == thread->sched)
	{
		thread->sched = NULL;
	}
}

/* When signaling for the first time, the thread must have the time budget allocated for 
   the first time to complete the task, and then notify the node that it is activated.
   When signaling the second time, the binding thread must want to receive the signal,
   so the notification node must be waiting. Therefore, the time budget may be needed 
   at this time, so it is necessary to donate the time budget */
void send_signal(notifation_t *node)
{
	struct ktcb *thread;
	struct tcb_queue queue;
	struct ktcb *dest;

	assert(node != NULL);

	thread = node->bindedtcb;
	
	LOCKED(&ipc_lock)
	{
		switch (node->state)
		{
			case notifation_state_idle:
				/* Check if we are bound and that thread is waiting for a message ~ for interrupt */
				if (thread)
				{
					if (thread->base.thread_state.obj_state == state_recv_blocked_state)
					{
						/* Send and start thread running */
						cancel_ipc(thread);
						set_thread_state(thread, state_queued_state);
	
						/* make can be scheduled */
						possible_donate_context(thread, node);
						
						if (is_schedulable(thread))
						{
							possible_switchto(thread);
						}
					}
					else
					{
						node->state = notifation_state_active;
					}
				}
				else
				{
					node->state = notifation_state_active;
				}
				break;
			case notifation_state_wait:

				queue = node->queue;
				dest = queue.head;
	
				assert(dest);
				
				queue = message_dequeue(dest, queue);
				node->queue = queue;
	
				if (!queue.head)
				{
					node->state = notifation_state_idle;
				}
				
				dest->notifation_node = node;
				set_thread_state(dest, state_queued_state);
				
				/* make can be scheduled */
				possible_donate_context(thread, node);
				
				if (is_schedulable(dest))
				{
					possible_switchto(dest);
				}
				break;
			case notifation_state_active:
				user_error("dest - %d has already active, dont active again\n", dest->thread_id);
				break;
			default:
				break;
		}
	}
}

/* receive process should be first called if you want to blcoked by notice */
/* receive process should be second called if you want to make notice idle */
void recevie_signal(struct ktcb *thread, notifation_t *node, bool_t blocking)
{
	assert(thread != NULL);
	assert(node != NULL);

	struct tcb_queue queue;

	LOCKED(&ipc_lock)
	{
		switch (node->state)
		{
			case notifation_state_idle:
			case notifation_state_wait:
	
				if (blocking)
				{
					set_thread_state(thread, state_notify_blocked_state);
	
					/* make cannot be scheduled */
					possible_reset_context(thread, node);
					
					schedule_tcb(thread);
	
					queue = node->queue;
					queue = message_append(thread, queue);
	
					node->state = notifation_state_wait;
					node->queue = queue;
					thread->notifation_node = node;
				}
				break;
			case notifation_state_active:
				node->state = notifation_state_idle;
				/* make can be scheduled */
				possible_donate_context(thread, node);
				break;
		}
	}
}

void cancel_signal(struct ktcb *thread, notifation_t *node)
{
	struct tcb_queue queue;

	assert(node->state == notifation_state_wait && thread != NULL && node != NULL);

	LOCKED(&ipc_lock)
	{
		queue = node->queue;
		queue = message_dequeue(thread,   queue);
		node->queue = queue;
		
		if (!queue.head)
		{
			node->state = notifation_state_idle;
		}
		
		set_thread_state(thread,   state_restart_state);
	}
}

void complete_signal(struct ktcb *thread, notifation_t *node)
{
	assert(thread  != NULL && node != NULL);
	
	LOCKED(&ipc_lock)
	{
		if (thread && node->state == notifation_state_active)
		{
			node->state = notifation_state_idle;
			thread->notifation_node = node;
		}
		else
		{
			user_error("tried to complete signal with inactive notice object\n");
		}
	}
}

void reorder_noticenode(struct ktcb *thread)
{
	assert(thread != NULL);
	assert(thread->notifation_node != NULL);

	struct tcb_queue queue;
	notifation_t *node = thread->notifation_node;
	
	LOCKED(&ipc_lock)
	{
		queue = node->queue;
		queue = message_dequeue(thread,   queue);
		queue = message_append(thread,	queue);
		node->queue = queue;
	}
}

static void exchange_ipc_timeout(u16_t t)
{
	message_time_t timeout = { .raw = t };
	ticks_t ticks = 
		us_to_ticks(message_time_period_m(timeout) << message_time_period_e(timeout));
	
	set_deadline(ticks, &(_current_thread->thread_id));
}

exception_t do_exchange_ipc(	
	word_t recv_gid, 
	word_t send_gid,
	word_t timeout,
	word_t *send_any_gid)
{
	struct ktcb *r_thread = get_thread(recv_gid);
	struct ktcb *s_thread = get_thread(send_gid);

	message_time_t send_timeout = 
		{ .raw = timeout & 0x0000FFFF };
	message_time_t recv_timeout = 
		{ .raw = timeout & 0xFFFF0000 >> 16 };
	message_time_t wait_timeout = 
		{ .raw = timeout };
	
#if(0)
	if (inv_level != exchange_ipc)
	{
		user_error("IPC Object: Illegal operation attempted.");
		current_syscall_error_code = TCR_INVAL_PARA;
		return EXCEPTION_SYSCALL_ERROR;
	}
#endif

#if defined(CONFIG_USERSPACE)
#endif

	/* send and receive both are self */
	if (recv_gid == GLOBALID_NILTHREAD && 
		send_gid == GLOBALID_NILTHREAD)
	{
		if (wait_timeout.raw != 0) 
		{
			exchange_ipc_timeout(wait_timeout.raw);
		}
		else
		{
			set_thread_state(_current_thread, state_restart_state);
			user_error("IPC Object: Illegal operation IPC not exist - ids are both NIL.");
			current_syscall_error_code = IPC_NOT_EXIST;
			return EXCEPTION_SYSCALL_ERROR;
		}
	}
		
	/* send and receive both are self */
	if (recv_gid == GLOBALID_ANYTHREAD && 
		send_gid == GLOBALID_ANYTHREAD)
	{
		set_thread_state(_current_thread, state_restart_state);
		user_error("IPC Object: Illegal operation IPC not exist - ids are both ANY.");
		current_syscall_error_code = IPC_NOT_EXIST;
		return EXCEPTION_SYSCALL_ERROR;
	}

	/* Design experience: 
		As far as possible, the receiver first sends an application that needs a message, 
		and then the sender will immediately send the message to the receiver through the 
		fast path, while ensuring that the length of the message is within 6 words. In 
		other cases, it is a slow path */
		
	/* The IPC process is as follows: 
		1. the sender first calls IPC and uses {sid(nil), rid(cur)} to send itself, 
		the receiver then calls IPC and uses {sid(send), recv(nil)} to receive the message; 
		2. the receiver first Call IPC and use {sid(cur), rid(nil)} to receive itself, 
		the sender then calls IPC and uses {sid(nil), recv(receive)} to send a message */
		
	/* send IPC process - send_gid = nil(current), send_gid = any */
	if (recv_gid != GLOBALID_NILTHREAD)
	{
		/* INT configuration */
		if (recv_gid == TID_TO_GLOBALID(id_irq_request_id))
		{
			/* myself sends request to me */
			interrupt_request(_current_thread);
			return EXCEPTION_NONE;
		}

		if (!r_thread)
		{
			user_error("IPC Object: Illegal operation receive thread.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}

		/* self or direct communition */
		if (send_gid != GLOBALID_NILTHREAD) 	
		{
			if (!s_thread)
			{
				user_error("IPC Object: Illegal operation send thread.");
				current_syscall_error_code = TCR_INVAL_PARA;
				return EXCEPTION_SYSCALL_ERROR;
			}
			
			message_exchange(s_thread, r_thread);
			return EXCEPTION_NONE;
		}

		/* set timeout */
		if (send_timeout.raw != 0) 
		{
			exchange_ipc_timeout(send_timeout.raw);
		}

		/* send process: receive thread may be self or others */
		send_ipc(_current_thread, TRUE, TRUE, r_thread->message_node);
	}

	/* receive IPC process - recv_gid = nil(current), recv_gid = any */
	if (send_gid != GLOBALID_NILTHREAD)
	{		
		/* INT ack */
		if (send_gid == TID_TO_GLOBALID(id_irq_ack_id))
		{
			/* myself receives respond from me */
			interrupt_respond(_current_thread);
			return EXCEPTION_NONE;
		}
		
		if (!s_thread)
		{
			user_error("IPC Object: Illegal operation send thread.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}

		/* set receive timeout */
		if (recv_timeout.raw != 0) 
		{
			exchange_ipc_timeout(recv_timeout.raw);
		}	

		/* receive process: send thread may be self or others */
		receive_ipc(_current_thread, TRUE, s_thread->message_node);
	}	

	/* recv_gid != any, recv_gid = nil(current) */
	if (send_gid == GLOBALID_ANYTHREAD)
	{
		for (word_t thread_index = 0; thread_index < record_thread_count; thread_index ++)
		{
			s_thread = record_threads[thread_index];
			
			if (!s_thread)
			{
				user_error("IPC Object: Illegal operation any send thread.");
				current_syscall_error_code = TCR_INVAL_PARA;
				return EXCEPTION_SYSCALL_ERROR;
			}
			
			if (get_thread_object_state(s_thread) == state_send_blocked_state)
			{
				if (recv_timeout.raw != 0) 
				{
					exchange_ipc_timeout(recv_timeout.raw);
				}

				if (recv_gid != GLOBALID_NILTHREAD) 
					receive_ipc(r_thread, TRUE, s_thread->message_node);
				else
					receive_ipc(_current_thread, TRUE, s_thread->message_node);
				
				*send_any_gid = s_thread->thread_id;
				break;
			}
		}
	}

	/* schedule(); */
	/* reschedule_unlocked(); */
	
	return EXCEPTION_NONE;
}


exception_t syscall_exchange_ipc(
	word_t recv_gid, 
	word_t send_gid,
	word_t timeout,
	word_t *send_any_gid)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		fastipc_caller.receive = recv_gid;
		fastipc_caller.send = send_gid;
		fastipc_caller.timeout = timeout;
		fastipc_caller.anysend = send_any_gid;
		
		scheduler_action = SCHEDULER_ACTION_CHOOSE_PRIV_THREAD;
		
		set_privilege_status(priv_fastipc_priv, priv_ready_priv);
		
		schedule();
		reschedule_unlocked();
		return EXCEPTION_NONE;

	}

	return EXCEPTION_FAULT;
}
