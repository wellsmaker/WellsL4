#include <sys/string.h>
#include <device.h>
#include <model/atomic.h>
#include <linker/linker_defs.h>
#include <state/statedata.h>
#include <object/untyped.h>
#include <kernel/thread.h>
#include <api/syscall.h>
/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the DEVICE_INIT() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */

void do_device_config(s32_t level)
{
	struct device *config_obj;
	static struct device *config_levels[] = 
	{
		__device_pre_kernel_1_start,
		__device_pre_kernel_2_start,
		__device_post_kernel_start,
		__device_application_start,
		__device_init_end,
	};

	sys_device_level = level;
	
	for (config_obj = config_levels[level]; 
	     config_obj < config_levels[level + 1]; 
		 config_obj ++)
	{
		s32_t is_inited;
		struct device_config *config_device = config_obj->config;

		is_inited = config_device->init(config_obj);
		if (is_inited != 0) 
		{
			/* Initialization failed. Clear the API struct so that
			 * device_get_binding() will not succeed for it.
			 */
			config_obj->driver_api = NULL;
			config_obj->driver_data = NULL;
		}
	}
}


struct device *do_device_binding(const char *name)
{
	struct device *config_obj;

	/* Split the search into two loops: in the common scenario, where
	 * device names are stored in ROM (and are referenced by the user
	 * with CONFIG_* macros), only cheap pointer comparisons will be
	 * performed.  Reserve string comparisons for a fallback.
	 */
	for (config_obj = __device_init_start; 
	     config_obj != __device_init_end; 
		 config_obj ++) 
	{
		if ((config_obj->driver_api != NULL) &&
		    (config_obj->config->name == name)) 
		{
			return config_obj;
		}
	}

	for (config_obj = __device_init_start; 
	     config_obj != __device_init_end; 
		 config_obj ++) 
	{
		if (config_obj->driver_api == NULL) 
		{
			continue;
		}

		if (strcmp(name, config_obj->config->name) == 0) 
		{
			return config_obj;
		}
	}

	return NULL;
}


struct device *syscall_device_binding(const char *name)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		struct device *dev;
#if defined(CONFIG_USERSPACE) 
		char name_copy[CONFIG_DEVICE_NAME_LEN];
		if (do_user_string_copy(name_copy, (char *)name, sizeof(name_copy)) != 0) 
		{
			return 0;
		}
#endif
		dev = do_device_binding(name);
	
	
		schedule();
		/* reschedule_unlocked(); */
		
		return dev;

	}
	return NULL;
}



#if defined(CONFIG_DEVICE_POWER_MANAGEMENT) 
int do_device_pm_ctrl_nop(struct device *unused_dev,
		       u32_t unused_ctrl,
		       void *unused_context,
		       device_pm_cb unused_cb,
		       void *unused_arg)
{
	ARG_UNUSED(unused_dev);
	ARG_UNUSED(unused_ctrl);
	ARG_UNUSED(unused_context);
	ARG_UNUSED(unused_cb);
	ARG_UNUSED(unused_arg);

	return 0;
}

void do_device_get(struct device **device_list, int *device_count)
{
	*device_list = __device_init_start;
	*device_count = __device_init_end - __device_init_start;
}


s32_t do_device_check_any_busy(void)
{
	s32_t i = 0;

	for (i = 0; i < __device_busy_end - __device_busy_start; i++) 
	{
		if (__device_busy_start[i] != 0U) 
		{
			return -EBUSY;
		}
	}
	
	return 0;
}

s32_t do_device_check_busy(struct device *dev)
{
	if (atomic_test_bit((const atomic_t *)__device_busy_start, 
		(dev - __device_init_start))) 
	{
		return -EBUSY;
	}
	return 0;
}

void do_device_set_busy(struct device *dev)
{
	atomic_set_bit((atomic_t *) __device_busy_start,
		(dev - __device_init_start));

}

void do_device_clear_busy(struct device *dev)
{
	atomic_clear_bit((atomic_t *) __device_busy_start, 
		(dev - __device_init_start));
}
#endif
