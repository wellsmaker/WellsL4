/* create any one new kernel object */
/* And the untyped use this to alloc a new kernel object */
#include <api/syscall.h>
#include <device.h>
#include <kernel/thread.h>
#include <kernel/cspace.h>
#include <object/anode.h>
#include <sys/string.h>
#include <sys/math_extras.h>
#include <sys/rb.h>
#include <sys/stdbool.h>
#include <sys/inttypes.h>
#include <sys/assert.h>
#include <object/objecttype.h>
#include <user/anode.h>
#include <model/spinlock.h>
#include <sys/rb.h>
#include <sys/dlist.h>
#include <kernel_object.h>

MEM_POOL_DEFINE(_heap_mem_pool, CONFIG_HEAP_MEM_POOL_MIN_SIZE, CONFIG_HEAP_MEM_POOL_SIZE, CONFIG_KERNEL_OBJECT_NUMBER, 4);

static enum obj_tag d_obj_type = obj_null_obj;
static spinlock_t d_obj_lock;       /* k_obj rbtree/dlist */
/* static spinlock_t d_free_lock;  */    /* d_object_free */

extern struct rbtree d_obj_rb;
extern sys_dlist_t d_obj_dlist;

#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

size_t get_k_object_size(enum obj_tag type, word_t untyped_obj_size)
{
	size_t ret = 0;

	if (type % 2) 
	{
		/* arch */
		switch (type)
		{
			case obj_frame_obj: ret = sizeof(struct thread_page); break;
			//case obj_page_table_obj: ret = sizeof(struct thread_page_table); break;
			default: ret = 0; break;
		}
	}
	else
	{
		switch (type) 
		{
			/* Non device/stack objects */
			case obj_message_obj: ret = sizeof(struct message); break;
			case obj_notification_obj: ret = sizeof(struct notifation); break;
			case obj_sched_context_obj: ret = sizeof(struct thread_sched); break;
			case obj_thread_obj: ret = sizeof(struct ktcb); break;
			case obj_irq_control_obj: ret = sizeof(struct interrupt); break;
			case obj_irq_handler_obj: ret = sizeof(word_t); break;
			case obj_domain_obj: ret = sizeof(struct dschedule); break;
			case obj_untyped_obj: ret = untyped_obj_size; break;
			case obj_time_obj: ret = sizeof(struct timer_event); break;
			case obj_device_obj: ret = sizeof(struct device); break;
			case obj_pager_obj: ret = sizeof(struct pager_context); break;
			default: ret = 0; break;
		}
	}

	return ret;
}

#if defined(CONFIG_LOG) 
const string k_object_type_to_str(enum obj_tag type)
{
	const string ret;

	if (type % 2)
	{
		switch (type) 
		{
			/* arch */
			case obj_frame_obj: ret = "thread page frame"; break;
			//case obj_page_table_obj: ret = "thread page table"; break;
			default: ret = 0; break;
		}
	}
	else
	{
		/* -fdata-sections doesn't work right except in very very recent
		* GCC and these literal strings would appear in the binary even if
		* k_object_type_to_str was omitted by the linker
		*/
		switch (type) 
		{
			/* Core kernel objects */
			/* Non device/stack objects */
			case obj_message_obj: ret = "thread message"; break;
			case obj_notification_obj: ret = "thread notification"; break;
			case obj_sched_context_obj: ret = "thread schedule context"; break;
			case obj_thread_obj: ret = "thread self"; break;
			case obj_irq_control_obj: ret = "irq control"; break;
			case obj_irq_handler_obj: ret = "irq handler"; break;
			case obj_domain_obj: ret = "thread domain"; break;
			case obj_untyped_obj: ret = "thread untyped space"; break;
			case obj_time_obj: ret = "thread time"; break;
			case obj_device_obj: ret = "thread device"; break;
			case obj_pager_obj: ret = "thread pager"; break;
			default: ret = "?"; break;
		}
	}
	
	return ret;
}
#else
#define k_object_type_to_str(...) 
#endif

struct k_object prepare_k_object_delete(struct k_object *k, bool_t is_final)
{
	struct k_object pre_ret;

	if (is_arch_k_obj(k))
	{
		/* arch */
	}

	switch (get_k_object_type(k))
	{
		case obj_message_obj:
			if (is_final) 
			{
				/* cannel all ipc process */
			}
			break;
		case obj_notification_obj:
			if (is_final)
			{
				/* cannel all notification process */
			}
			break;
		case obj_null_obj:
		case obj_untyped_obj:
		case obj_irq_control_obj:
		case obj_domain_obj:
			break;		
		/*
		case obj_cnode_obj:
			if (is_final) 
			{

			}
			break;
		*/
		case obj_thread_obj:
		case obj_pager_obj:
			if (is_final) 
			{

			}
			break;
		case obj_sched_context_obj:
			if (is_final) 
			{

			}
			break;
		case obj_irq_handler_obj:
			if (is_final) 
			{

			}
			break;
		case obj_time_obj:
			if (is_final)
			{

			}
			break;
		case obj_device_obj:
			if (is_final)
			{

			}
			break;
	}
	
	pre_ret = obj_null_obj_new();
	return pre_ret;
}

bool_t is_same_objectname(struct k_object *k1, struct k_object *k2)
{
	return k1->name == k2->name;
}

bool_t is_same_objecttype(struct k_object *k1, struct k_object *k2)
{
	switch (get_k_object_type(k1))
	{
		/*
		case obj_untyped_obj:
			if (get_k_object_type(k2) == obj_untyped_obj)
			{
				return is_same_objectname(k1, k2);
			}
			break;
		*/
		case obj_message_obj:
			if (get_k_object_type(k2) == obj_message_obj)
			{
				return is_same_objectname(k1, k2);
			}
			break;	
		case obj_notification_obj:
			if (get_k_object_type(k2) == obj_notification_obj)
			{
				return is_same_objectname(k1, k2);
			}			
			break;	
			/*
	    case obj_reply_obj:
			if (get_k_object_type(k2) == obj_reply_obj)
			{
				return is_same_objectname(k1, k2);

			}			
			break;	
			
	    case obj_cnode_obj:
			if (get_k_object_type(k2) == obj_cnode_obj)
			{
				return is_same_objectname(k1, k2);

			}			
			break;	
			*/
	    case obj_thread_obj:
			if (get_k_object_type(k2) == obj_thread_obj)
			{
				return is_same_objectname(k1, k2);

			}			
			break;	
		case obj_pager_obj:
			if (get_k_object_type(k2) == obj_pager_obj)
			{
				return is_same_objectname(k1, k2);

			}	
			break;
	    case obj_irq_control_obj:
			if (get_k_object_type(k2) == obj_irq_control_obj ||
				get_k_object_type(k2) == obj_irq_handler_obj)
			{
				return true;

			}			
			break;	
	    case obj_irq_handler_obj:
			if (get_k_object_type(k2) == obj_irq_handler_obj)
			{
				return is_same_objectname(k1, k2);

			}			
			break;		
		case obj_sched_context_obj:
			if (get_k_object_type(k2) == obj_sched_context_obj)
			{
				return is_same_objectname(k1, k2);

			}
			break;
	    case obj_domain_obj:
			if (get_k_object_type(k2) == obj_domain_obj)
			{
				return true;

			}		
			break;	
		case obj_time_obj:
			break;
		case obj_device_obj:
			break;
		default:
			if (is_arch_k_obj(k1) && is_arch_k_obj(k2))
			{

			}
			break;
	}

	return false;
}

bool_t is_same_object(struct k_object *k1, struct k_object *k2)
{
	if (get_k_object_type(k1) == obj_untyped_obj)
	{
		return false;
	}

	if (get_k_object_type(k1) == obj_irq_control_obj && 
		get_k_object_type(k2) == obj_irq_handler_obj)
	{
		return false;
	}
		
	return is_same_objecttype(k1, k2);
}


void *d_object_alloc(enum obj_tag type, word_t untyped_obj_size)
{
	struct d_object *d_obj;

	if (type % 2)
	{
		/* arch */
	}
	
	d_obj = malloc_object(sizeof(*d_obj) + get_k_object_size(type, untyped_obj_size));
	if (!d_obj)
	{
		return NULL;
	}

	d_obj->k_obj.name = (char *)&d_obj->k_obj_self;
	d_obj->k_obj.type = type;
	d_obj->k_obj.flag |= obj_allocated_obj;
	d_obj->k_obj.right = 0;
	d_obj->k_obj.data = 0;

	memset((char *)&d_obj->k_obj_self, 0, get_k_object_size(type, untyped_obj_size));
	sys_dnode_init(&d_obj->k_obj_dlnode);
	rb_node_init(&d_obj->k_obj_rbnode);
	
	if (d_obj_type != type)
	{
		LOCKED(&d_obj_lock)
		{
			rb_insert(&d_obj_rb, &d_obj->k_obj_rbnode);
			sys_dlist_append(&d_obj_dlist, &d_obj->k_obj_dlnode);
		}
	}

	d_obj_type = type;
	
	return d_obj->k_obj.name;
}

void d_object_free(void *obj)
{
	struct d_object *d_obj;

	/* This function is intentionally not exposed to user mode.
	 * There's currently no robust way to track that an object isn't
	 * being used by some other thread
	 */
	d_obj = d_object_find(obj);

	if (d_obj != NULL)
	{
		d_object_delete(d_obj);
		free_object(d_obj);
	}
}

/* data and flag is the link change */
void k_object_update_data(bool_t is_set,
				uintptr_t data, struct k_object *k)
{
	if (is_arch_k_obj(k))
	{
		
	}
	
	if (is_set)
	{
		if (POPCOUNTL(k->data) == 0) 
		{
			k->data |= data;
			k->flag |= obj_granted_obj;
		}
	}
	else
	{
		if (POPCOUNTL(k->data) == 1) 
		{
			k->data &= ~data;
			k->flag &= ~obj_granted_obj;
		}
	}

	/* update special object function */
	switch (get_k_object_type(k))
	{
		case obj_untyped_obj:
			break;
		case obj_message_obj:
			break;
		case obj_notification_obj:
			break;
		case obj_thread_obj:
			break;
		case obj_sched_context_obj:
			break;
		case obj_irq_control_obj:
			break;
		case obj_irq_handler_obj:
			break;
		case obj_domain_obj:
			break;
		case obj_time_obj:
			break;
		case obj_device_obj:
			break;
		case obj_pager_obj:
			break;
	}
}

void k_object_mask_right(bool_t is_set,
				uintptr_t right, struct k_object *k)
{
	if (is_arch_k_obj(k))
	{
		
	}
	if (is_set)
	{
		k->right |= right;
	}
	else
	{
		k->right &= ~right;
	}
	/* update special object function */
	switch (get_k_object_type(k))
	{
		case obj_untyped_obj:
			break;
		case obj_message_obj:
			break;
		case obj_notification_obj:
			break;
		case obj_thread_obj:
			break;
		case obj_sched_context_obj:
			break;
		case obj_irq_control_obj:
			break;
		case obj_irq_handler_obj:
			break;
		case obj_domain_obj:
			break;
		case obj_time_obj:
			break;
		case obj_device_obj:
			break;
		case obj_pager_obj:
			break;
	}
}

void *syscall_dobject_alloc(enum obj_tag otype, word_t untyped_obj_size)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		void *obj;
#if defined(CONFIG_USERSPACE)
		SYSCALL_OOPS(SYSCALL_VERIFY_MSG(otype > obj_any_obj && otype < obj_last_obj,
						"bad object type %d requested", otype));
		SYSCALL_OOPS(SYSCALL_VERIFY_MSG(otype == obj_untyped_obj && untyped_obj_size != 0,
						"bad object size %d requested", otype));
#endif
	
		obj = d_object_alloc(otype, untyped_obj_size);
	
		schedule();
		/* reschedule_unlocked(); */
	
		return obj;
	}
	return NULL;
}

void syscall_dobject_free(void *obj)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if defined(CONFIG_USERSPACE)
		
		SYSCALL_OOPS(SYSCALL_VERIFY_MSG(obj != NULL,
						"bad object ptr %p requested", obj));
#endif
	
		d_object_free(obj);
	
		schedule();
		/* reschedule_unlocked(); */

	}
}

static sword_t init_memory_pool_module(struct device *dev)
{
	ARG_UNUSED(dev);
	
	STRUCT_SECTION_FOREACH(k_mem_pool, pool_ptr) 
	{
		sys_mem_pool_base_init(&pool_ptr->base);
	}
	return 0;
}

sword_t alloc_slot(struct k_mem_pool *pool_ptr, struct slot *slot_ptr, size_t size)
{
	word_t level;
	word_t block;

	if (sys_mem_pool_block_alloc(&pool_ptr->base, size, &level, &block, &slot_ptr->data)
		== 0)
	{
		slot_ptr->desc.offset = poolptr_to_offset(pool_ptr);
		slot_ptr->desc.level = level;
		slot_ptr->desc.block = block;

		return 0;
	}

	return -EAGAIN;
}

void free_slot(struct slot *slot_ptr)
{
	struct slot_desc *desc = &slot_ptr->desc;
	struct k_mem_pool *pool = offset_to_poolptr(desc->offset);

	sys_mem_pool_block_free(&pool->base, desc->level, desc->block);
}

void *malloc_object(size_t size)
{
	struct slot slot_entity;

	/*
	 * get a block large enough to hold an initial (hidden) block
	 * descriptor, as well as the space the caller requested
	 */
	if (size_add_overflow(size, WB_UP(sizeof(struct slot_desc)), &size))
	{
		return NULL;
	}
	
	if (alloc_slot(&_heap_mem_pool, &slot_entity, size) != 0) 
	{
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	(void)memcpy(slot_entity.data, &slot_entity.desc, sizeof(struct slot_desc));

	/* return address of the user area part of the block to the caller */
	return (byte_t *)slot_entity.data + WB_UP(sizeof(struct slot_desc));
}

void *calloc_object(size_t nmemb, size_t size)
{
	size_t bounds;
	void * ret;

	if (size_mul_overflow(nmemb, size, &bounds))
	{
		return NULL;
	}
	
	ret = malloc_object(bounds);

	if (ret)
	{
		(void) memset(ret, 0, bounds);
		return ret;
	}
	
	return NULL;
}

void free_object(void *obj_ptr)
{
	if (obj_ptr != NULL) 
	{
		/* point to hidden block descriptor at start of block */
		struct slot_desc *desc_ptr = (struct slot_desc *)
			((char *)obj_ptr - WB_UP(sizeof(struct slot_desc)));

		/* return block to the heap memory pool */
		struct k_mem_pool *pool = offset_to_poolptr(desc_ptr->offset);

		sys_mem_pool_block_free(&pool->base, desc_ptr->level, desc_ptr->block);
	}
}

SYS_INIT(init_memory_pool_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
