#include <types_def.h>
#include <kernel/thread.h>
#include <kernel/cspace.h>
#include <sys/string.h>
#include <sys/math_extras.h>
#include <sys/rb.h>
#include <api/syscall.h>
#include <device.h>
#include <sys/stdbool.h>
#include <sys/inttypes.h>
#include <object/cnode.h>
#include <model/spinlock.h>
#include <kernel/thread.h>

/* The originally synchronization strategy made heavy use of recursive
 * irq_locking, which ports poorly to spinlocks which are
 * non-recursive.  Rather than try to redesign as part of
 * spinlockification, this uses multiple locks to preserve the
 * original semantics exactly.  The locks are named for the data they
 * protect where possible, or just for the code that uses them where
 * not.
 */

static spinlock_t k_obj_lock;       /* k_obj struct data */

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)


/* you can static use macro to do it of collect the set 
   that thread and object set */
/* the way is that access and grant ~ kernel object of flag and data */

uintptr_t get_thread_k_object_data(void *object)
{
	struct k_object *ko = k_object_find(object);
	return (ko != NULL && ko->data != 0) ? ko->data : 0;
}


void k_object_access_grant(void *object, struct ktcb *thread)
{
	struct k_object *ko = k_object_find(object);
	uintptr_t grant = get_thread_k_object_data(thread);

	if (ko != NULL && grant != 0) 
	{
		if (POPCOUNTL(ko->data) == 0)
		{
			ko->data |= grant;
			ko->flag |= obj_granted_obj;
		}
	}
}

void k_object_access_revoke(void *object, struct ktcb *thread)
{
	struct k_object *ko = k_object_find(object);
	uintptr_t grant = get_thread_k_object_data(thread);

	if (ko != NULL && grant != 0) 
	{
		if (POPCOUNTL(ko->data) == 1) 
		{
			ko->data &= ~grant;
			ko->flag &= ~obj_granted_obj;
		}
	}
}

sword_t k_object_access_validate(struct k_object *ko, struct ktcb *thread,
						enum obj_tag otype)
{
	struct k_object *curr_ko = k_object_find(thread);
	
	if (unlikely(ko == NULL || ko->name != ko ||
			(otype != obj_any_obj && ko->type != otype)))
	{
		return -EBADF;
	}

	if (unlikely(ko->flag & obj_init_obj || 
			!(ko->flag & obj_allocated_obj)))
	{
		return -EINVAL;
	}

	/* Manipulation of any kernel k_objects by a user thread requires that
	 * thread be granted access first, even for uninitialized k_objects
	 */
	if (unlikely(curr_ko == NULL || (ko->data & curr_ko->data) == 0 ||
			POPCOUNTL(ko->data) > 1))
	{
		return -EPERM;
	}

	if (unlikely(ko->type   != obj_thread_obj && 
			!(ko->flag & obj_granted_obj)))
	{
		return -EINVAL;
	}
	
	return 0;
}


/* Normally these would be included in userspace.c, but the way
 * syscall_dispatch.c declares weak handlers results in build errors if these
 * are located in userspace.c. Just put in a separate file.
 *
 * To avoid double k_object_find() lookups, we don't call the implementation
 * function, but call a level deeper.
 */
void syscall_kobject_access_grant(void *object, struct ktcb *thread)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if defined(CONFIG_USERSPACE) 
		struct k_object *ko;
		/* thread */
		SYSCALL_OOPS(SYSCALL_OBJ(thread, obj_thread_obj));
		/* grant */
		LOCKED(&k_obj_lock)
		{
			k_object_access_grant(object, thread);
		}
		/* object */
		ko = validate_any_k_object(object, thread);
		SYSCALL_OOPS(SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied", object));
#else
		LOCKED(&k_obj_lock)
		{
			k_object_access_grant(object, thread);
		}
#endif
	
		schedule();
		reschedule_unlocked();

	}
}

void syscall_kobject_access_revoke(void *object, struct ktcb *thread)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if defined(CONFIG_USERSPACE) 
		struct k_object *ko;
		/* thread */
		SYSCALL_OOPS(SYSCALL_OBJ(thread, obj_thread_obj));
		/* object */
		ko = validate_any_k_object(object, thread);
		SYSCALL_OOPS(SYSCALL_VERIFY_MSG(ko != NULL, "object %p access denied", object));
		/* revoke */
		LOCKED(&k_obj_lock)
		{
			k_object_access_revoke(ko, thread);
		}
#else 
		LOCKED(&k_obj_lock)
		{
			k_object_access_revoke(object, thread);
		}
#endif
	
	
		schedule();
		reschedule_unlocked();

	}
}
