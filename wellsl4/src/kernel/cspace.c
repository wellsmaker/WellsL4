#include <sys/string.h>
#include <sys/math_extras.h>
#include <sys/rb.h>
#include <sys/util.h>
#include <api/syscall.h>
#include <device.h>
#include <sys/stdbool.h>
#include <user/anode.h>
#include <sys/inttypes.h>
#include <kernel/cspace.h>
#include <sys/assert.h>
#include <object/tcb.h>
#include <types_def.h>
#include <api/errno.h>
#include <model/preemption.h>
/*
 * Heap is defined using HEAP_MEM_POOL_SIZE configuration option.
 *
 * This module defines the heap memory pool and the _HEAP_MEM_POOL symbol
 * that has the address of the associated memory pool struct.
 */

/* you must remeber that the kernel object way is here */

static spinlock_t d_obj_lock;       /* k_obj rbtree/dlist */
static spinlock_t d_free_lock;      /* d_object_free */

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

static bool_t k_node_lessthan(struct rbnode *a, struct rbnode *b);

/*
 * Red/black tree of allocated kernel k_objects, for reasonably fast lookups
 * based on object pointer values.
 */
struct rbtree d_obj_rb = {
	.lessthan_fn = k_node_lessthan 
};

/*
 * Linked list of allocated kernel k_objects, for iteration over all allocated
 * k_objects (and potentially deleting them during iteration).
 */
sys_dlist_t d_obj_dlist = 
	SYS_DLIST_STATIC_INIT(&d_obj_dlist);

/*
 * TODO: Write some hash table code that will replace both d_obj_rb
 * and k_obj_dlnode.
 */

static bool_t k_node_lessthan(struct rbnode *a, struct rbnode *b)
{
	return a < b;
}

word_t get_k_object_type(struct k_object *k_obj)
{
	return k_obj->type;
}

bool_t is_arch_k_obj(struct k_object *k)
{
	return (get_k_object_type(k) % 2);
}

bool_t is_empty_d_object(struct d_object *d)
{
	return get_k_object_type(&d->k_obj) == obj_null_obj;
}


bool_t is_final_d_object(struct d_object *d) /* for same object */
{
	if (sys_dnode_is_llinked(&d_obj_dlist, &d->k_obj_dlnode))
	{
		return true;
	}
	else
	{
		sys_dnode_t *next = sys_dlist_peek_next(&d_obj_dlist, &d->k_obj_dlnode);
		struct d_object *next_d_obj = CONTAINER_OF(next, struct d_object, k_obj_dlnode);
		
		if (!is_same_object(&d->k_obj, &next_d_obj->k_obj))
		{
			return true;
		}
	}

	return false;
}

bool_t is_parent_d_object(struct d_object *d1, struct d_object *d2) /* d2 is d1 next */
{
	if (!is_same_object(&d1->k_obj, &d2->k_obj))
	{
		return false;
	}

	return true;
}

bool_t is_d_object_no_child(struct d_object *d)
{
	if (sys_dlist_peek_next(&d_obj_dlist, &d->k_obj_dlnode))
	{
		sys_dnode_t *next = sys_dlist_peek_next(&d_obj_dlist, &d->k_obj_dlnode);
		struct d_object *next_d_obj = CONTAINER_OF(next, struct d_object, k_obj_dlnode);

		if (is_parent_d_object(d, next_d_obj))
		{
			return false;
		}
	}

	return true;
}


static struct d_object *node_to_d_obj(struct rbnode *d_obj_rbnode)
{
	return CONTAINER_OF(d_obj_rbnode, struct d_object, k_obj_rbnode);
}

struct d_object *d_object_find(void *obj) /* k_obj_self ? */
{
	struct rbnode *d_obj_rbnode;
	struct d_object *ret;

	/* For any dynamically allocated kernel object, the object
	 * pointer is just a member of the conatining struct d_object,
	 * so just a little arithmetic is necessary to locate the
	 * corresponding struct rbnode
	 */
	d_obj_rbnode = (struct rbnode *)((char *)obj - sizeof(struct rbnode));

	LOCKED(&d_obj_lock)
	{
		if (rb_contains(&d_obj_rb, d_obj_rbnode))
		{
			ret = node_to_d_obj(d_obj_rbnode);
		} 
		else 
		{
			ret = NULL;
		}
	}

	return ret;
}

struct k_object *k_object_find(void *obj)
{
	struct k_object *ret = NULL;
	struct d_object *d_obj;

	d_obj = d_object_find(obj);
	
	if (d_obj != NULL)
	{
		ret = &d_obj->k_obj;
	}

	return ret;
}

/* same object type */
void d_object_insert(struct    k_object *new_k, struct d_object *src_d, struct d_object *dest_d)
{
	LOCKED(&d_obj_lock)
	{
		if (!new_k)
		{
			dest_d->k_obj = *new_k;
			memcpy((char *)&dest_d->k_obj_self, (char *)new_k->name, 
				get_k_object_size(new_k->type, 0));
		}

		if (&dest_d->k_obj)
		{
			assert(get_k_object_type(&dest_d->k_obj) == obj_null_obj);
			assert(!sys_dnode_is_linked(&dest_d->k_obj_dlnode));
			assert(!rb_contains(&d_obj_rb, &dest_d->k_obj_rbnode));
			
			rb_insert(&d_obj_rb, &dest_d->k_obj_rbnode);
			sys_dlist_insert(&src_d->k_obj_dlnode, &dest_d->k_obj_dlnode); //after insert and src is dest parent
		}
	}
}

void d_object_move(struct k_object *new_k, struct d_object *src_d, struct d_object *dest_d)
{
	LOCKED(&d_obj_lock)
	{
		assert(get_k_object_type(&dest_d->k_obj) == obj_null_obj);
		assert(!sys_dnode_is_linked(&dest_d->k_obj_dlnode));
		assert(!rb_contains(&d_obj_rb, &dest_d->k_obj_rbnode));

		if (!new_k)
		{
			dest_d->k_obj = *new_k;
			memcpy((char *)&dest_d->k_obj_self, (char *)new_k->name, 
				get_k_object_size(new_k->type, 0));
		}
		else
		{
			dest_d->k_obj = src_d->k_obj;
			memcpy((char *)&dest_d->k_obj_self, (char *)src_d->k_obj.name, 
				get_k_object_size(src_d->k_obj.type, 0));
		}

		src_d->k_obj = obj_null_obj_new();

		if (&dest_d->k_obj)
		{
			rb_remove(&d_obj_rb, &src_d->k_obj_rbnode);
			sys_dlist_remove(&src_d->k_obj_dlnode);
			
			rb_insert(&d_obj_rb, &dest_d->k_obj_rbnode);
			sys_dlist_insert(&src_d->k_obj_dlnode, &dest_d->k_obj_dlnode); //after insert
		}
	}
}

void d_object_swap(struct k_object *k1, struct k_object *k2, 
					struct d_object *d1, struct d_object *d2)
{
	LOCKED(&d_obj_lock)
	{
		size_t tmp_size_k1 = get_k_object_size(k1->type, 0);
		size_t tmp_size_k2 = get_k_object_size(k2->type, 0);

		if (tmp_size_k1 == tmp_size_k2)
		{
			d1->k_obj = *k2;
			d1->k_obj = *k1;
		}
	}
}

void d_object_swap_of(struct d_object *d1, struct d_object *d2)
{
	struct k_object k1, k2;

	if (d1 == d2)
	{
		return;
	}

	k1 = d1->k_obj;
	k2 = d2->k_obj;

	d_object_swap(&k1, &k2, d1, d2);
}

exception_t d_object_revoke(struct d_object *d)
{
	struct d_object *next;
	exception_t status = EXCEPTION_NONE;
	
	LOCKED(&d_obj_lock)
	{
		for (next = SYS_DLIST_PEENEXT_CONTAINER(&d_obj_dlist, d, k_obj_dlnode);
			next && is_parent_d_object(d, next);
			next = SYS_DLIST_PEENEXT_CONTAINER(&d_obj_dlist, d, k_obj_dlnode))
		{
			status = d_object_delete(d);

			if (status != EXCEPTION_NONE)
			{
				return status;
			}

			status = preemption_point();

			if (status != EXCEPTION_NONE)
			{
				return status;
			}
		}
	}

	return status;
}

exception_t d_object_delete(struct d_object *d)
{
	exception_t ret;

	ret = prepare_d_object_delete(d);

	if (ret != EXCEPTION_NONE)
	{
		return ret;
	}
	else
	{
		empty_d_object(d);
		LOCKED(&d_free_lock)
		{
			rb_remove(&d_obj_rb, &d->k_obj_rbnode);
			sys_dlist_remove(&d->k_obj_dlnode);
		}
	}
	
	return EXCEPTION_NONE;
}


exception_t prepare_d_object_delete(struct d_object *d)
{
	bool_t is_final;
	struct k_object ret;
	exception_t status;
	
	while (get_k_object_type(&d->k_obj) != obj_null_obj)
	{
		is_final = is_final_d_object(d);
		ret = prepare_k_object_delete(&d->k_obj, is_final);

		if (is_k_object_removable(&ret))
		{
			return EXCEPTION_NONE;
		}

		status = preemption_point();

		if (status != EXCEPTION_NONE)
		{
			return status;
		}
	}

	return EXCEPTION_NONE;
}

void k_object_wordlist_foreach(wordlist_cb_func_t func, void *para)
{
	struct d_object *obj, *next_obj;
	
	LOCKED(&d_obj_lock)
	{
		SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&d_obj_dlist,
			obj, next_obj, k_obj_dlnode) 
		{
			func(&obj->k_obj, para);
		}
	}
}


