#ifndef CNODE_H_
#define CNODE_H_


#include <types_def.h>
#include <kernel/cspace.h>
#include <sys/assert.h>
#include <sys/errno.h>
#include <object/objecttype.h>
#include <kernel_object.h>


#ifdef __cplusplus
extern "C" {
#endif

#if(0)
/* kernel object set with a thread */
struct k_object_set
{
	struct ktcb *thread; /* thread to */
	void * const *k_objects; /* thread with some k_objects */
};

typedef struct k_object_set obj_set_t;

struct k_object_map
{
	struct ktcb *src_t; /* from thread map */
	struct ktcb *dest_t; /* to thread map */
	struct k_object *k_obj; /* kernel object map */
};

typedef struct k_object_map obj_map_t;

/**
 * @brief Grant a static thread access to a list of kernel k_objects
 *
 * For record_threads declared with THREAD_DEFINE(), grant the thread access to
 * a set of kernel k_objects. These k_objects do not need to be in an initialized
 * state. The permissions will be granted when the record_threads are initialized
 * in the early boot sequence.
 *
 * All arguments beyond the first must be pointers to kernel k_objects.
 *
 * @param name_ Name of the thread, as passed to THREAD_DEFINE()
 */

/* t - thread address , or s - set name */
#define THREAD_OBJECT_SET(thread, set, ...) \
	static void * const _CONCAT(_object_list_, set)[] = { \
			__VA_ARGS__, NULL }; \
	static const STRUCT_SECTION_ITERABLE(k_object_set, \
			_CONCAT(_object_access_, set)) = { \
			(thread), \
			(_CONCAT(_object_list_, set)) \
	}

#endif

uintptr_t get_thread_k_object_data(void *object);
void k_object_access_grant(void *object, struct ktcb *thread);
void k_object_access_revoke(void *object, struct ktcb *thread);
sword_t k_object_access_validate(struct k_object *ko, struct ktcb *thread,
						enum obj_tag otype);

/*
__syscall void kobject_access_grant(void *object, struct ktcb *thread)
{


}
__syscall void kobject_access_revoke(void *object, struct ktcb *thread)
{


}
*/

static FORCE_INLINE void k_object_dump(sword_t retval, void *obj, enum obj_tag type)
{
	switch (retval) 
	{
		case -EBADF:
			user_error("%p is not a valid %s", obj, k_object_type_to_str(type));
			break;
		case -EPERM:
			user_error("%p has not a owner %s", obj, k_object_type_to_str(type));
			break;
		case -EINVAL:
			user_error("%p used without be granted", obj, k_object_type_to_str(type));
			break;
		default:
			/* Not error */
			break;
	}
}

static FORCE_INLINE struct k_object *validate_any_k_object(void *obj, 
					struct ktcb *thread)
{
	struct k_object *ko;
	sword_t pass;

	ko = k_object_find(obj);

	/* This can be any kernel object and it doesn't have to be
	 * initialized
	 */
	pass = k_object_access_validate(ko, thread, obj_any_obj);
	if (pass != 0) 
	{
#if defined(CONFIG_LOG) 
		k_object_dump(pass, obj, obj_any_obj);
#endif
		return NULL;
	}

	return ko;
}

static FORCE_INLINE sword_t obj_validation_check(struct k_object *ko,
					void *obj, enum obj_tag otype)
{
	sword_t pass;

	pass = k_object_access_validate(ko, obj, otype);

#if defined(CONFIG_LOG) 
	if (pass != 0) 
	{
		k_object_dump(pass, obj, otype);
	}
#else
	ARG_UNUSED(obj);
#endif

	return pass;
}

#ifdef __cplusplus
}
#endif

#endif
