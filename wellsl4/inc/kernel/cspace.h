#ifndef KERNEL_CSPACE_H_
#define KERNEL_CSPACE_H_


#include <types_def.h>
#include <toolchain.h>
#include <sys/dlist.h>
#include <sys/rb.h>
#include <object/objecttype.h>
#include <api/errno.h>
#include <sys/string.h>
#include <kernel_object.h>

#ifdef __cplusplus
extern "C" {
#endif


enum obj_status 
{
	obj_init_obj = BIT(0), /* null object */
	obj_allocated_obj = BIT(1), /* object other */
	obj_granted_obj = BIT(2), /* access granted object for one thread */
	obj_subsystem_obj = BIT(3),
};

typedef void (*wordlist_cb_func_t)(struct k_object *ko, void *ctx);

word_t get_k_object_type(struct k_object *k_obj);
bool_t is_arch_k_obj(struct k_object *k);
bool_t is_empty_d_object(struct d_object *d);
bool_t is_final_d_object(struct d_object *d);
bool_t is_parent_d_object(struct d_object *d1, struct d_object *d2);
bool_t is_d_object_no_child(struct d_object *d);
struct d_object *d_object_find(void *obj);
struct k_object *k_object_find(void *obj);
void d_object_insert(struct    k_object *new_k, struct d_object *src_d, struct d_object *dest_d);
void d_object_move(struct k_object *new_k, struct d_object *src_d, struct d_object *dest_d);
void d_object_swap(struct k_object *k1, struct k_object *k2, 
					struct d_object *d1, struct d_object *d2);

void d_object_swap_of(struct d_object *d1, struct d_object *d2);
exception_t d_object_revoke(struct d_object *d);
exception_t d_object_delete(struct d_object *d);
exception_t prepare_d_object_delete(struct d_object *d);
void k_object_wordlist_foreach(wordlist_cb_func_t func, void *ctx_ptr);

static FORCE_INLINE struct k_object obj_null_obj_new(void)
{
	struct k_object obj;

	obj.name = NULL;
	obj.type = obj_null_obj;
	obj.flag = obj_init_obj;
	obj.data = 0;
	obj.right = 0;

	return obj;
}

static FORCE_INLINE void empty_d_object(struct d_object *d_obj)
{		
	if (get_k_object_type(&d_obj->k_obj) == obj_null_obj)
	{
		d_obj->k_obj = obj_null_obj_new();
		memset((char *)&d_obj->k_obj_self, 0, get_k_object_size(obj_null_obj, 0));
		sys_dnode_init(&d_obj->k_obj_dlnode);
		rb_node_init(&d_obj->k_obj_rbnode);
	}
}

static FORCE_INLINE bool_t is_k_object_removable(struct k_object *k_obj)
{
	switch (get_k_object_type(k_obj))
	{
		case obj_null_obj:
			return true;
		default:
			return false;
	}
}

#ifdef __cplusplus
}
#endif

#endif
