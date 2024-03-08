#ifndef OBJECTTYPE_H_
#define OBJECTTYPE_H_

#include <types_def.h>
#include <sys/mempool.h>
#include <arch/cpu.h>
#include <kernel_object.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Note on sizing: the use of a 20 bit field for block means that,
 * assuming a reasonable minimum block size of 16 bytes, we're limited
 * to 16M of memory managed by a single pool.  Long term it would be
 * good to move to a variable bit size based on configuration.
 */
struct slot_desc 
{
	u32_t offset  : 8;
	u32_t level   : 4;
	u32_t block   : 20;
};

struct slot
{
	struct slot_desc desc;
	void *data;
};

struct k_mem_pool
{
	struct sys_mem_pool_base base;
};

/**
 * @brief Statically define and initialize a memory pool.
 *
 * The memory pool's buffer contains @a n_max blocks that are @a max_size bytes
 * long. The memory pool allows blocks to be repeatedly partitioned into
 * quarters, down to blocks of @a min_size bytes long. The buffer is aligned
 * to a @a align -byte boundary.
 *
 * If the pool is to be accessed outside the module where it is defined, it
 * can be declared via
 *
 * @code extern struct k_mem_pool <name>; @endcode
 *
 * @param name Name of the memory pool.
 * @param minsz Size of the smallest blocks in the pool (in bytes).
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align Alignment of the pool's buffer (power of 2).
 * @req K-MPOOL-001
 */
#define MEM_POOL_DEFINE(name, minsz, maxsz, nmax, align)		\
	char __aligned(WB_UP(align)) _mpool_buf_##name[WB_UP(maxsz) * nmax \
				  + _MPOOL_BITS_SIZE(maxsz, minsz, nmax)]; \
	struct sys_mem_pool_lvl _mpool_lvls_##name[MPOOL_LVLS(maxsz, minsz)]; \
	STRUCT_SECTION_ITERABLE(k_mem_pool, name) = { \
		.base = {						\
			.buf = _mpool_buf_##name,			\
			.max_sz = WB_UP(maxsz),				\
			.n_max = nmax,					\
			.n_levels = MPOOL_LVLS(maxsz, minsz),		\
			.levels = _mpool_lvls_##name,			\
			.flag = SYS_MEM_POOL_KERNEL			\
		} \
	}; \
	BUILD_ASSERT(WB_UP(maxsz) >= _MPOOL_MINBLK)

extern struct k_mem_pool _k_mem_pool_list_start[];
extern struct k_mem_pool _k_mem_pool_list_start[];

static FORCE_INLINE struct k_mem_pool *offset_to_poolptr(word_t offset)
{
	return &_k_mem_pool_list_start[offset];
}

static FORCE_INLINE word_t poolptr_to_offset(struct k_mem_pool *pool_ptr)
{
	return pool_ptr - &_k_mem_pool_list_start[0];
}

static FORCE_INLINE void k_object_right_set(struct k_object *k, uintptr_t r)
{
	sys_bitfield_set_bit((maddr_t)&k->right, r);
}

static FORCE_INLINE void k_object_right_clear(struct k_object *k, uintptr_t r)
{
	sys_bitfield_clear_bit((maddr_t)&k->right, r);
}

static FORCE_INLINE void k_object_right_clear_all(uintptr_t r)
{
	k_object_wordlist_foreach(k_object_right_clear, r);
}

static FORCE_INLINE sword_t k_object_right_test(struct k_object *k, uintptr_t r)
{
	return sys_bitfield_test_bit((maddr_t)&k->right, r);
}

static FORCE_INLINE void k_object_data_set(struct k_object *k, uintptr_t d)
{
	sys_bitfield_set_bit((maddr_t)&k->data, d);
}

static FORCE_INLINE void k_object_data_clear(struct k_object *k, uintptr_t d)
{
	sys_bitfield_clear_bit((maddr_t)&k->data, d);
}

static FORCE_INLINE void k_object_data_clear_all(uintptr_t d)
{
	k_object_wordlist_foreach(k_object_data_clear, d);
}

static FORCE_INLINE sword_t k_object_data_test(struct k_object *k, uintptr_t d)
{
	return sys_bitfield_test_bit((maddr_t)&k->data, d);
}


size_t get_k_object_size(enum obj_tag type, word_t untyped_obj_size);
const string k_object_type_to_str(enum obj_tag type);
struct k_object prepare_k_object_delete(struct k_object *k, bool_t is_final);
bool_t is_same_objectname(struct k_object *k1, struct k_object *k2);
bool_t is_same_objecttype(struct k_object *k1, struct k_object *k2);
bool_t is_same_object(struct k_object *k1, struct k_object *k2);
void *d_object_alloc(enum obj_tag type, word_t untyped_obj_size);
void d_object_free(void *obj);
void k_object_update_data(bool_t is_set,
				uintptr_t data, struct k_object *k);
void k_object_mask_right(bool_t is_set,
				uintptr_t right, struct k_object *k);
/*
__syscall void *dobject_alloc(enum obj_tag otype, word_t untyped_obj_size)
{
	return NULL;
}
__syscall void dobject_free(void *obj)
{

}
*/
sword_t alloc_slot(struct k_mem_pool *pool_ptr, struct slot *slot_ptr, size_t size);
void free_slot(struct slot *slot_ptr);
void *malloc_object(size_t size);
void *calloc_object(size_t nmemb, size_t size);
void free_object(void *obj_ptr);

#ifdef __cplusplus
}
#endif

#endif
