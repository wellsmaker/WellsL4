#include <types_def.h>
#include <object/untyped.h>
#include <object/objecttype.h>
#include <kernel/cspace.h>
#include <sys/errno.h>
#include <state/statedata.h>
#include <api/syscall.h>
#include <model/preemption.h>
#include <object/tcb.h>
#include <kernel/thread.h>

static sword_t user_copy(void *dst, const void *src, size_t size, bool is_to)
{
	sword_t ret = EFAULT;

	/* Does the caller in user mode have access to this memory? */
	if (is_to ? SYSCALL_MEMORY_WRITE(dst, size) : SYSCALL_MEMORY_READ(src, size)) 
	{
		return ret;
	}

	(void)memcpy(dst, src, size);
	ret = 0;
	
	return ret;
}

sword_t do_user_from_copy(void *dst, const void *src, size_t size)
{
	return user_copy(dst, src, size, false);
}

sword_t do_user_to_copy(void *dst, const void *src, size_t size)
{
	return user_copy(dst, src, size, true);
}

sword_t do_user_string_copy(char *dst, const char *src, size_t maxlen)
{
	size_t actual_len;
	sword_t ret, err;

	actual_len = user_string_nlen(src, maxlen, &err);
	
	if (err != 0 || actual_len == maxlen || size_add_overflow(actual_len, 1, &actual_len)) 
	{
		user_error("Untyped Object: Illegal operation attempted.");
		ret = EINVAL;
		return ret;
	}
	
	ret = do_user_from_copy(dst, src, actual_len);
	dst[actual_len - 1] = '\0';

	return ret;
}


/* the untyped is the memory of you can put any type object , so you need give a user type and size */
static exception_t reset_untyped_object(struct d_object *src, size_t src_size)
{
	exception_t status;

	memset((char *)&src->k_obj_self, 0, get_k_object_size(src->k_obj.type, src_size));
	status = preemption_point();
	if (status != EXCEPTION_NONE)
	{
		return status;
	}
	
	return EXCEPTION_NONE;
}

exception_t retype_untyped_object(struct d_object *dest, enum obj_tag user_type,
	bool_t reset, size_t user_size, struct d_object *src)
{
	exception_t status;

	if (reset)
	{
		status = reset_untyped_object(src, user_size);
		if (status != EXCEPTION_NONE)
		{
			return status;
		}
	}

	if (src == NULL)
	{
		dest = src;
		dest->k_obj.type = user_type;
		return EXCEPTION_NONE;
	}
	
	dest = d_object_alloc(user_type, user_size);
	if (dest == NULL) 
	{
		return EXCEPTION_LOOKUP_FAULT;
	}

	(void)memcpy(dest->k_obj.name, src->k_obj.name, user_size);
	return EXCEPTION_NONE;
}


exception_t syscall_retype_untyped(void *kobject, 
	size_t user_size, enum obj_tag user_type)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		sword_t errno;
		bool_t reset;
		word_t obj_size;
		exception_t status;
		struct d_object *src = NULL;
		struct d_object *dest = NULL;
#if(0)
		if (inv_level != memory_control)
		{
			user_error("Untyped Object: Illegal operation attempted.");
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
		if (user_type >= obj_last_obj)
		{
			user_error("Untyped Retype: Invalid object type.");
			return EXCEPTION_SYSCALL_ERROR;
		}
	
		obj_size = get_k_object_size(user_type, user_size);
		/* check obj_size is right */
	
		if (kobject == NULL)
		{
			kobject = d_object_alloc(obj_untyped_obj, user_size);
			if (kobject == NULL)
			{
				return EXCEPTION_LOOKUP_FAULT;
			}
		}
		
		src = d_object_find(kobject);
		if (src == NULL)
		{
			user_error("Untyped Retype: Invalid destination address.");
			return EXCEPTION_SYSCALL_ERROR;
		}
		else
		{
#if defined(CONFIG_USERSPACE)
			size_t actual_len;
	
			actual_len = user_string_nlen(src->k_obj.name, user_size, &errno);		
			if (errno != 0 || actual_len == user_size || \
				size_add_overflow(actual_len, 1, &actual_len))
			{
				user_error("Untyped Object: Illegal operation attempted.");
				return EXCEPTION_SYSCALL_ERROR;
			}
			
			/* Does the caller in user mode have access to read this memory? */
			if (SYSCALL_MEMORY_READ(src->k_obj.name, user_size))
			{
				user_error("Untyped Object: Illegal operation attempted.");
				return EXCEPTION_SYSCALL_ERROR;
			}
#endif
		}
		
		if (is_d_object_no_child(src))
		{
			reset = false;
		}
		else
		{
			reset = true;
		}
		
		set_thread_state(_current_thread, state_restart_state);
		status = retype_untyped_object(dest, user_type, reset, obj_size, src);
		if (status != EXCEPTION_NONE)
		{
			return status;
		}
		
#if defined(CONFIG_USERSPACE)
		if (SYSCALL_MEMORY_WRITE(dest->k_obj.name, user_size))
		{
			user_error("Untyped Object: Illegal operation attempted.");
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
	
	
		schedule();
		/* reschedule_unlocked(); */
	
		return EXCEPTION_NONE;

	}
	return EXCEPTION_FAULT;
}
