/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYS_MEMPOOL_BASE_H_
#define SYS_MEMPOOL_BASE_H_

#include <types_def.h>
#include <toolchain.h>
#include <sys/util.h>

/*
 * Definitions and macros used by both the IRQ-safe k_mem_pool and user-mode
 * compatible sys_mem_pool implementations
 */

struct sys_mem_pool_lvl {
	union {
		u32_t *bits_p;
		u32_t bits[sizeof(u32_t *)/4];
	};
	sys_dlist_t free_list;
};

#define SYS_MEM_POOL_KERNEL	BIT(0)
#define SYS_MEM_POOL_USER	BIT(1)

struct sys_mem_pool_base {
	void *buf;
	size_t max_sz;
	u16_t n_max;
	byte_t n_levels;
	s8_t max_inline_level;
	struct sys_mem_pool_lvl *levels;
	byte_t flag;
};

struct sys_mem_pool {
	struct sys_mem_pool_base base;
};

struct sys_mem_pool_block {
	struct sys_mem_pool *pool;
	u32_t level : 4;
	u32_t block : 28;
};

/**
 * @brief Statically define system memory pool
 *
 * The memory pool's buffer contains @a n_max blocks that are @a max_size bytes
 * long. The memory pool allows blocks to be repeatedly partitioned into
 * quarters, down to blocks of @a min_size bytes long. The buffer is aligned
 * to a @a align -byte boundary.
 *
 * If the pool is to be accessed outside the module where it is defined, it
 * can be declared via
 *
 * @code extern struct sys_mem_pool <name>; @endcode
 *
 * This pool will not be in an initialized state. You will still need to
 * run sys_mem_pool_init() on it before using any other APIs.
 *
 * @param name Name of the memory pool.
 * @param ignored ignored, any value
 * @param minsz Size of the smallest blocks in the pool (in bytes).
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align Alignment of the pool's buffer (power of 2).
 * @param section Destination binary section for pool data
 */
#define SYS_MEM_POOL_DEFINE(name, ignored, minsz, maxsz, nmax, align, section) \
	char __aligned(WB_UP(align)) GENERIC_SECTION(section)		\
		_mpool_buf_##name[WB_UP(maxsz) * nmax			\
				  + _MPOOL_BITS_SIZE(maxsz, minsz, nmax)]; \
	struct sys_mem_pool_lvl GENERIC_SECTION(section)		\
		_mpool_lvls_##name[MPOOL_LVLS(maxsz, minsz)];		\
	GENERIC_SECTION(section) struct sys_mem_pool name = {		\
		.base = {						\
			.buf = _mpool_buf_##name,			\
			.max_sz = WB_UP(maxsz),				\
			.n_max = nmax,					\
			.n_levels = MPOOL_LVLS(maxsz, minsz),		\
			.levels = _mpool_lvls_##name,			\
			.flags = SYS_MEM_POOL_USER			\
		}							\
	};								\
	BUILD_ASSERT(WB_UP(maxsz) >= _MPOOL_MINBLK)

#define _MPOOL_MINBLK sizeof(sys_dnode_t)
#define MPOOL_HAVE_LVL(maxsz, minsz, l) \
	(((maxsz) >> (2*(l))) >= MAX((minsz), _MPOOL_MINBLK) ? 1 : 0)
	
#define MPOOL_LVLS(maxsz, minsz)		\
	(MPOOL_HAVE_LVL((maxsz), (minsz), 0) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 1) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 2) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 3) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 4) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 5) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 6) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 7) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 8) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 9) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 10) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 11) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 12) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 13) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 14) +	\
	MPOOL_HAVE_LVL((maxsz), (minsz), 15))

/* Rounds the needed bits up to integer multiples of u32_t */
#define MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l) \
	((((n_max) << (2*(l))) + 31) / 32)

/* One or two 32-bit words gets stored free unioned with the pointer,
 * otherwise the calculated unclamped value
 */
#define MPOOL_LBIT_WORDS(n_max, l)					 \
	(MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l) <= sizeof(u32_t *)/4 ? 0 \
	 : MPOOL_LBIT_WORDS_UNCLAMPED(n_max, l))

/* How many bytes for the bitfields of a single level? */
#define MPOOL_LBIT_BYTES(maxsz, minsz, l, n_max)	\
	(MPOOL_HAVE_LVL((maxsz), (minsz), (l)) ?	\
	 4 * MPOOL_LBIT_WORDS((n_max), l) : 0)

/* Size of the bitmap array that follows the buffer in allocated memory */
#define _MPOOL_BITS_SIZE(maxsz, minsz, n_max) \
	(MPOOL_LBIT_BYTES(maxsz, minsz, 0, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 1, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 2, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 3, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 4, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 5, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 6, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 7, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 8, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 9, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 10, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 11, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 12, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 13, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 14, n_max) +	\
	MPOOL_LBIT_BYTES(maxsz, minsz, 15, n_max))


void sys_mem_pool_base_init(struct sys_mem_pool_base *p);
sword_t sys_mem_pool_block_alloc(struct sys_mem_pool_base *p, size_t size,
			      u32_t *level_p, u32_t *block_p, void **data_p);
void sys_mem_pool_block_free(struct sys_mem_pool_base *p, u32_t level,
			      u32_t block);

/**
 * @brief Initialize a memory pool
 *
 * This is intended to complete initialization of memory pools that have been
 * declared with SYS_MEM_POOL_DEFINE().
 *
 * @param p Memory pool to initialize
 */
static FORCE_INLINE void sys_mem_pool_init(struct sys_mem_pool *p)
{
	sys_mem_pool_base_init(&p->base);
}

/**
 * @brief Allocate a block of memory
 *
 * Allocate a chunk of memory from a memory pool. This cannot be called from
 * interrupt context.
 *
 * @param p Address of the memory pool
 * @param size Requested size of the memory block
 * @return A pointer to the requested memory, or NULL if none is available
 */
void *sys_mem_pool_alloc(struct sys_mem_pool *p, size_t size);

/**
 * @brief Free memory allocated from a memory pool
 *
 * Free memory previously allocated by sys_mem_pool_alloc().
 * It is safe to pass NULL to this function, in which case it is a no-op.
 *
 * @param ptr Pointer to previously allocated memory
 */
void sys_mem_pool_free(void *ptr);

/**
 * @brief Try to perform in-place expansion of memory allocated from a pool
 *
 * Return 0 if memory previously allocated by sys_mem_pool_alloc()
 * can accommodate a new size, otherwise return the size of data that
 * needs to be copied over to new memory.
 *
 * @param ptr Pointer to previously allocated memory
 * @param new_size New size requested for the memory block
 * @return A 0 if OK, or size of data to copy elsewhere
 */
size_t sys_mem_pool_try_expand_inplace(void *ptr, size_t new_size);

#endif
