#include <sys/onoff.h>

static spinlock_t onoff_lock;
#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

extern bool_t is_thread_in_isr(void);
		
#define CLIENT_NOTIFY_METHOD_MASK 0x03
#define CLIENT_VALID_FLAGS_MASK 0x07
#define SERVICE_CONFIG_FLAGS	     \
	(ONOFF_SERVICE_START_SLEEPS  \
	 | ONOFF_SERVICE_STOP_SLEEPS \
	 | ONOFF_SERVICE_RESET_SLEEPS)
#define SERVICE_REFS_MAX UINT16_MAX
#define SERVICE_STATE_OFF 0
#define SERVICE_STATE_ON ONOFF_SERVICE_INTERNAL_BASE
#define SERVICE_STATE_TRANSITION (ONOFF_SERVICE_INTERNAL_BASE << 1)
#define SERVICE_STATE_TO_ON (SERVICE_STATE_TRANSITION | SERVICE_STATE_ON)
#define SERVICE_STATE_TO_OFF (SERVICE_STATE_TRANSITION | SERVICE_STATE_OFF)
#define SERVICE_STATE_MASK (SERVICE_STATE_ON | SERVICE_STATE_TRANSITION)

static void set_service_state(struct onoff_service *srv, u32_t state)
{
	srv->flag &= ~SERVICE_STATE_MASK;
	srv->flag |= (state & SERVICE_STATE_MASK);
}

static sword_t validate_args(const struct onoff_service *srv, struct onoff_client *cli)
{
	if ((srv == NULL) || (cli == NULL)) 
	{
		return -EINVAL;
	}

	sword_t rv = 0;
	u32_t mode = cli->flag;

	/* Reject unexpected flag. */
	if (mode != (cli->flag & CLIENT_VALID_FLAGS_MASK))
	{
		return -EINVAL;
	}

	/* Validate configuration based on mode */
	switch (mode & CLIENT_NOTIFY_METHOD_MASK) 
	{
		case ONOFF_CLIENT_NOTIFY_SPINWAIT:
			break;
		case ONOFF_CLIENT_NOTIFY_CALLBACK:
			if (cli->async.callback.handler == NULL) 
			{
				rv = -EINVAL;
			}
			break;
#ifdef CONFIG_POLL	
		case ONOFF_CLIENT_NOTIFY_SIGNAL:
			if (cli->async.signal == NULL) 
			{
				rv = -EINVAL;
			}
			break;
#endif
		default:
			rv = -EINVAL;
			break;
	}

	/* Clear the result here instead of in all callers. */
	if (rv == 0)
	{
		cli->result = 0;
	}

	return rv;
}

sword_t onoff_service_init(struct onoff_service *srv,
		       onoff_service_transition_fn start,
		       onoff_service_transition_fn stop,
		       onoff_service_transition_fn reset,
		       u32_t flag)
{
	if ((flag & SERVICE_CONFIG_FLAGS) != flag) 
	{
		return -EINVAL;
	}

	if ((start == NULL) || (stop == NULL))
	{
		return -EINVAL;
	}

	*srv = (struct onoff_service)ONOFF_SERVICE_INITIALIZER(start, stop, reset, flag);

	return 0;
}

static void notify_one(struct onoff_service *srv,
		       struct onoff_client *cli,
		       sword_t res)
{
	word_t flag = cli->flag;

	/* Store the result, and notify if requested. */
	cli->result = res;
	cli->flag = 0;
	switch (flag & CLIENT_NOTIFY_METHOD_MASK)
	{
		case ONOFF_CLIENT_NOTIFY_SPINWAIT:
			break;
		case ONOFF_CLIENT_NOTIFY_CALLBACK:
			cli->async.callback.handler(srv, cli, cli->async.callback.user_data, res);
			break;
#ifdef CONFIG_POLL
		case ONOFF_CLIENT_NOTIFY_SIGNAL:
			/* k_poll_signal_raise(cli->async.signal, res); */
			break;
#endif /* CONFIG_POLL */
		default:
			assert(false);
	}
}

static void notify_all(struct onoff_service *srv, sys_slist_t *list, sword_t res)
{
	while (!sys_slist_is_empty(list)) 
	{
		sys_snode_t *node = sys_slist_get_not_empty(list);
		struct onoff_client *cli = CONTAINER_OF(node, struct onoff_client, node);

		notify_one(srv, cli, res);
	}
}

static void onoff_start_notify(struct onoff_service *srv, sword_t res)
{
	
	sys_slist_t clients = srv->clients;

	/* Can't have a queued releaser during start */
	assert(srv->releaser == NULL);

	LOCKED(&onoff_lock)
	{
		/* If the start failed log an error and leave the rest of the
		 * state in place for diagnostics.
		 *
		 * If the start succeeded record a reference for all clients
		 * and set the state to ON.  There must be at least one client
		 * left to receive the result.
		 *
		 * In either case reset the client queue and notify all
		 * clients of operation completion.
		 */
		if (res < 0)
		{
			srv->flag &= ~SERVICE_STATE_TRANSITION;
			srv->flag |= ONOFF_SERVICE_HAS_ERROR;
		} 
		else 
		{
			sys_snode_t *node;
			word_t refs = 0U;
	
			set_service_state(srv, SERVICE_STATE_ON);
	
			SYS_SLIST_FOR_EACH_NODE(&clients, node) 
			{
				refs += 1U;
			}
	
			/* Update the reference count, or fail if the count
			 * would overflow.
			 */
			if (srv->refs > (SERVICE_REFS_MAX - refs))
			{
				srv->flag |= ONOFF_SERVICE_HAS_ERROR;
			}
			else 
			{
				srv->refs += refs;
			}
			
			assert(srv->refs > 0U);
		}
	
		sys_slist_init(&srv->clients);
	}

	notify_all(srv, &clients, res);
}

sword_t onoff_request(struct onoff_service *srv, struct onoff_client *cli)
{
	bool add_client = false;        /* add client to pending list */
	bool start = false;             /* invoke start transition */
	bool notify = false;            /* do client notification */
	sword_t rv = validate_args(srv, cli);

	if (rv < 0)
	{
		return rv;
	}

	LOCKED(&onoff_lock)
	{
		if ((srv->flag & ONOFF_SERVICE_HAS_ERROR) != 0)
		{
			rv = -EIO;
			goto out;
		}
	
		/* Reject if this would overflow the reference count. */
		if (srv->refs == SERVICE_REFS_MAX)
		{
			rv = -EAGAIN;
			goto out;
		}
	
		u32_t state = srv->flag & SERVICE_STATE_MASK;
	
		switch (state) 
		{
			case SERVICE_STATE_TO_OFF:
				/* Queue to start after release */
				assert(srv->releaser != NULL);
				add_client = true;
				rv = 3;
				break;
			case SERVICE_STATE_OFF:
				/* Reject if in a non-thread context and start could
				 * wait.
				 */
				if ((is_thread_in_isr()) /* || is_pre_kernel() */
					&& ((srv->flag & ONOFF_SERVICE_START_SLEEPS) != 0U)) 
				{
					rv = -EWOULDBLOCK;
					break;
				}
		
				/* Start with first request while off */
				assert(srv->refs == 0);
					
				set_service_state(srv, SERVICE_STATE_TO_ON);
				start = true;
				add_client = true;
				rv = 2;
				break;
			case SERVICE_STATE_TO_ON:
				/* Already starting, just queue it */
				add_client = true;
				rv = 1;
				break;
			case SERVICE_STATE_ON:
				/* Just increment the reference count */
				notify = true;
				break;
			default:
				rv = -EINVAL;
				break;
		}
	
	out:
		if (add_client) 
		{
			sys_slist_append(&srv->clients, &cli->node);
		}
		else if (notify) 
		{
			srv->refs += 1;
		}
	}

	if (start) 
	{
		assert(srv->start != NULL);
		srv->start(srv, onoff_start_notify);
	} 
	else if (notify) 
	{
		notify_one(srv, cli, 0);
	}

	return rv;
}

static void onoff_stop_notify(struct onoff_service *srv, sword_t res)
{
	bool notify_clients = false;
	sword_t client_res = res;
	bool start = false;
	sys_slist_t clients = srv->clients;
	struct onoff_client *releaser = srv->releaser;

	LOCKED(&onoff_lock)
	{
		/* If the stop operation failed log an error and leave the
		 * rest of the state in place.
		 *
		 * If it succeeded remove the last reference and transition to
		 * off.
		 *
		 * In either case remove the last reference, and notify all
		 * waiting clients of operation completion.
		 */
		if (res < 0) 
		{
			srv->flag &= ~SERVICE_STATE_TRANSITION;
			srv->flag |= ONOFF_SERVICE_HAS_ERROR;
			notify_clients = true;
		} 
		else if (sys_slist_is_empty(&clients))
		{
			set_service_state(srv, SERVICE_STATE_OFF);
		} 
		else if ((is_thread_in_isr()) /* || is_pre_kernel() */
			   && ((srv->flag & ONOFF_SERVICE_START_SLEEPS) != 0U)) 
		{
			set_service_state(srv, SERVICE_STATE_OFF);
			notify_clients = true;
			client_res = -EWOULDBLOCK;
		} 
		else 
		{
			set_service_state(srv, SERVICE_STATE_TO_ON);
			start = true;
		}
	
		assert(releaser);
		srv->refs -= 1U;
		srv->releaser = NULL;
		assert(srv->refs == 0);
	
		/* Remove the clients if there was an error or a delayed start
		 * couldn't be initiated, because we're resolving their
		 * operation with an error.
		 */
		if (notify_clients) 
		{
			sys_slist_init(&srv->clients);
		}

	}

	/* Notify the releaser.  If there was an error, notify any
	 * pending requests; otherwise if there are pending requests
	 * start the transition to ON.
	 */
	notify_one(srv, releaser, res);
	if (notify_clients)
	{
		notify_all(srv, &clients, client_res);
	} 
	else if (start) 
	{
		srv->start(srv, onoff_start_notify);
	}
}

sword_t onoff_release(struct onoff_service *srv, struct onoff_client *cli)
{
	bool stop = false;      /* invoke stop transition */
	bool notify = false;    /* do client notification */
	sword_t rv = validate_args(srv, cli);

	if (rv < 0) 
	{
		return rv;
	}

	LOCKED(&onoff_lock)
	{
		if ((srv->flag & ONOFF_SERVICE_HAS_ERROR) != 0)
		{
			rv = -EIO;
			goto out;
		}
	
		u32_t state = srv->flag & SERVICE_STATE_MASK;
	
		switch (state) 
		{
		case SERVICE_STATE_ON:
			/* Stay on if release leaves a client. */
			if (srv->refs > 1U)
			{
				notify = true;
				rv = 1;
				break;
			}
	
			/* Reject if in non-thread context but stop could
			 * wait
			 */
			if ((is_thread_in_isr()) /*   || is_pre_kernel() */
				&& ((srv->flag & ONOFF_SERVICE_STOP_SLEEPS) != 0)) 
			{
				rv = -EWOULDBLOCK;
				break;
			}
	
			stop = true;
	
			set_service_state(srv, SERVICE_STATE_TO_OFF);
			srv->releaser = cli;
			rv = 2;
	
			break;
		case SERVICE_STATE_TO_ON:
			rv = -EBUSY;
			break;
		case SERVICE_STATE_OFF:
		case SERVICE_STATE_TO_OFF:
			rv = -EALREADY;
			break;
		default:
			rv = -EINVAL;
		}
	
	out:
		if (notify)
		{
			srv->refs -= 1U;
		}
	}

	if (stop) 
	{
		assert(srv->stop != NULL);
		srv->stop(srv, onoff_stop_notify);
	}
	else if (notify) 
	{
		notify_one(srv, cli, 0);
	}

	return rv;
}

static void onoff_reset_notify(struct onoff_service *srv, sword_t res)
{
	sys_slist_t clients = srv->clients;

	LOCKED(&onoff_lock)
	{
		/* If the reset failed clear the transition flag but otherwise
		 * leave the state unchanged.
		 *
		 * If it was successful clear the reference count and all
		 * flag except capability flag (sets to SERVICE_STATE_OFF).
		 */
		if (res < 0)
		{
			srv->flag &= ~SERVICE_STATE_TRANSITION;
		} 
		else
		{
			assert(srv->refs == 0U);
			srv->refs = 0U;
			srv->flag &= SERVICE_CONFIG_FLAGS;
		}
		
		sys_slist_init(&srv->clients);

	}

	notify_all(srv, &clients, res);
}

sword_t onoff_service_reset(struct onoff_service *srv, struct onoff_client *cli)
{
	if (srv->reset == NULL)
	{
		return -ENOTSUP;
	}

	bool reset = false;
	sword_t rv = validate_args(srv, cli);

	if (rv < 0) 
	{
		return rv;
	}

	/* Reject if in a non-thread context and reset could wait. */
	if ((is_thread_in_isr()) /* || is_pre_kernel() */
	    && ((srv->flag & ONOFF_SERVICE_RESET_SLEEPS) != 0U)) 
	{
		return -EWOULDBLOCK;
	}

	LOCKED(&onoff_lock)
	{
		if ((srv->flag & ONOFF_SERVICE_HAS_ERROR) == 0) 
		{
			rv = -EALREADY;
			goto out;
		}
	
		if ((srv->flag & SERVICE_STATE_TRANSITION) == 0) 
		{
			reset = true;
			srv->flag |= SERVICE_STATE_TRANSITION;
		}
	
	out:
		if (rv >= 0) 
		{
			sys_slist_append(&srv->clients, &cli->node);
		}

	}
	
	if (reset) 
	{
		srv->reset(srv, onoff_reset_notify);
	}

	return rv;
}

sword_t onoff_cancel(struct onoff_service *srv, struct onoff_client *cli)
{
	sword_t rv = validate_args(srv, cli);

	if (rv < 0) 
	{
		return rv;
	}

	rv = -EALREADY;

	LOCKED(&onoff_lock)
	{
		u32_t state = srv->flag & SERVICE_STATE_MASK;
	
		/* Can't remove the last client waiting for the in-progress
		 * transition, as there would be nobody to receive the
		 * completion notification, which might indicate a service
		 * error.
		 */
		if (sys_slist_find_and_remove(&srv->clients, &cli->node)) 
		{
			rv = 0;
			if (sys_slist_is_empty(&srv->clients)
				&& (state != SERVICE_STATE_TO_OFF)) 
			{
				rv = -EWOULDBLOCK;
				sys_slist_append(&srv->clients, &cli->node);
			}
		} 
		else if (srv->releaser == cli) 
		{
			/* must be waiting for TO_OFF to complete */
			rv = -EWOULDBLOCK;
		}

	}

	if (rv == 0) 
	{
		notify_one(srv, cli, -ECANCELED);
	}

	return rv;
}
