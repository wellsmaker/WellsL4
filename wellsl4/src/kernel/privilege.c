#include <kernel/privilege.h>
#include <types_def.h>
#include <kernel_object.h>
#include <object/ipc.h>
#include <device.h>
#include <model/atomic.h>
#include <api/errno.h>
#include <state/statedata.h>
#include <kernel/thread.h>

static s32_t privilege_thread_status[priv_end_priv]; 

static s32_t init_privilege_status_module(struct device *dev)
{
	ARG_UNUSED(dev);

	word_t p_type = 0;
	for (; p_type < priv_end_priv; p_type++)
	{
		privilege_thread_status[p_type] = priv_inactive_priv;
	}
	
	return 0;
}

void set_privilege_status(word_t type, s32_t status)
{
	atomic_set(&privilege_thread_status[type], status);
}

s32_t get_privilege_status(word_t type)
{
	return atomic_get(&privilege_thread_status[type]);
}

exception_t do_health_monitor(void)
{
	return EXCEPTION_NONE;
}


exception_t do_debugging_monitor(void)
{
	return EXCEPTION_NONE;
}


void privilege_thread_entry(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (true)
	{
		word_t p_type;

privilege_retry:
		/* check status and handle current status */
		for (p_type = 0; p_type < priv_end_priv; p_type++)
		{
			if (get_privilege_status(p_type) == priv_ready_priv)
			{
				switch (p_type)
				{
					case priv_fastipc_priv:
						do_exchange_ipc(fastipc_caller.receive, fastipc_caller.send, 
							fastipc_caller.timeout, fastipc_caller.anysend);
						
						break;
					case priv_health_monitor_priv:
						do_health_monitor();
						break;
					case priv_debugging_monitor_priv:
						do_debugging_monitor();
						break;
					default:
						break;
				}

				set_privilege_status(p_type, priv_blocked_priv);
			}
		}

		/* handle new status during the checking process */
		for (p_type = 0; p_type < priv_end_priv; p_type++)
		{
			if (get_privilege_status(p_type) == priv_ready_priv)
				goto privilege_retry;
		}

		reschedule_required();
		schedule();
		reschedule_unlocked();
	}
}

SYS_INIT(init_privilege_status_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
