/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Red/Black balanced tree data structure
 *
 * This implements an intrusive balanced tree that guarantees
 * O(log2(N)) runtime for all operations and amortized O(1) behavior
 * for creation and destruction of whole trees.  The algorithms and
 * naming are conventional per existing academic and didactic
 * implementations, c.f.:
 *
 * https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * The implementation is size-optimized to prioritize runtime memory
 * usage.  The data structure is intrusive, which is to say the struct
 * rbnode handle is intended to be placed in a separate struct the
 * same way other such structures (e.g. WellL4's dlist list) and
 * requires no data pointer to be stored in the node.  The color bit
 * is unioned with a pointer (fairly common for such libraries).  Most
 * notably, there is no "parent" pointer stored in the node, the upper
 * structure of the tree being generated dynamically via a stack as
 * the tree is recursed.  So the overall memory overhead of a node is
 * just two pointers, identical with a doubly-linked list.
 */

#ifndef SYS_RB_H_
#define SYS_RB_H_

#ifndef _ASMLANGUAGE

/* Our SDK/toolchains integration seems to be inconsistent about
 * whether they expose alloca.h or not.  On gcc it's a moot point as
 * it's always builtin.
 */

#ifdef __GNUC__
#ifndef alloca
#define alloca __builtin_alloca
#endif
#else
#include <alloca.h>
#endif

#include <types_def.h>

struct rbnode {
	struct rbnode *children[2];
};

/* Theoretical maximum depth of tree based on pointer size. If memory
 * is filled with 2-pointer nodes, and the tree can be twice as a
 * packed binary tree, plus root...  Works out to 59 entries for 32
 * bit pointers and 121 at 64 bits.
 */
#define TBITS(t) ((sizeof(t)) < 8 ? 2 : 3)
#define PBITS(t) (8 * sizeof(t))
#define MAX_RBTREE_DEPTH (2 * (PBITS(int *) - TBITS(int *) - 1) + 1)

/**
 * @typedef rb_lessthan_t
 * @brief Red/black tree comparison predicate
 *
 * Compares the two nodes and returns true if node A is strictly less
 * than B according to the tree's sorting criteria, false otherwise.
 *
 * Note that during insert, the new node being inserted will always be
 * "A", where "B" is the existing node within the tree against which
 * it is being compared.  This trait can be used (with care!) to
 * implement "most/least recently added" semantics between nodes which
 * would otherwise compare as equal.
 */
typedef bool_t (*rb_lessthan_t)(struct rbnode *a, struct rbnode *b);

struct rbtree {
	struct rbnode *root;
	rb_lessthan_t lessthan_fn;
	int max_depth;
#ifdef CONFIG_MISRA_SANE
	struct rbnode *iter_stack[MAX_RBTREE_DEPTH];
	unsigned char iter_left[MAX_RBTREE_DEPTH];
#endif
};

typedef void (*rb_visit_t)(struct rbnode *node, void *cookie);

struct rbnode *rb_child(struct rbnode *node, int side);
int rb_is_black(struct rbnode *node);
#ifndef CONFIG_MISRA_SANE
void rb_walk_of(struct rbnode *node, rb_visit_t visit_fn, void *cookie);
#endif
struct rbnode *rb_get_minmax(struct rbtree *tree, int side);

void rb_node_init(struct rbnode *node);

/**
 * @brief Insert node into tree
 */
void rb_insert(struct rbtree *tree, struct rbnode *node);

/**
 * @brief Remove node from tree
 */
void rb_remove(struct rbtree *tree, struct rbnode *node);

/**
 * @brief Returns the lowest-sorted member of the tree
 */
static inline struct rbnode *rb_get_min(struct rbtree *tree)
{
	return rb_get_minmax(tree, 0);
}

/**
 * @brief Returns the highest-sorted member of the tree
 */
static inline struct rbnode *rb_get_max(struct rbtree *tree)
{
	return rb_get_minmax(tree, 1);
}

/**
 * @brief Returns true if the given node is part of the tree
 *
 * Note that this does not internally dereference the node pointer
 * (though the tree's lessthan callback might!), it just tests it for
 * equality with items in the tree.  So it's feasible to use this to
 * implement a "set" construct by simply testing the pointer value
 * itself.
 */
bool rb_contains(struct rbtree *tree, struct rbnode *node);

#ifndef CONFIG_MISRA_SANE
/**
 * @brief Walk/enumerate a rbtree
 *
 * Very simple recursive enumeration.  Low code size, but requiring a
 * separate function can be clumsy for the user and there is no way to
 * break out of the loop early.  See RB_FOR_EACH for an iterative
 * implementation.
 */
static inline void rb_walk(struct rbtree *tree, rb_visit_t visit_fn,
			   void *cookie)
{
	rb_walk_of(tree->root, visit_fn, cookie);
}
#endif

struct _rb_foreach {
	struct rbnode **stack;
	char *is_left;
	int top;
};

#ifdef CONFIG_MISRA_SANE
#define _RB_FOREACH_INIT(tree, node) {					\
	.stack   = &(tree)->iter_stack[0],				\
	.is_left = &(tree)->iter_left[0],				\
	.top     = -1							\
}
#else
#define _RB_FOREACH_INIT(tree, node) {					\
	.stack   = (struct rbnode **)					\
			alloca((tree)->max_depth * sizeof(struct rbnode *)), \
	.is_left = (char *)alloca((tree)->max_depth * sizeof(char)),		\
	.top     = -1							\
}
#endif

struct rbnode *rb_foreach_next(struct rbtree *tree, struct _rb_foreach *f);

/**
 * @brief Walk a tree in-order without recursing
 *
 * While @ref rb_walk() is very simple, recursing on the C stack can
 * be clumsy for some purposes and on some architectures wastes
 * significant memory in stack frames.  This macro implements a
 * non-recursive "foreach" loop that can iterate directly on the tree,
 * at a moderate cost in code size.
 *
 * Note that the resulting loop is not safe against modifications to
 * the tree.  Changes to the tree structure during the loop will
 * produce incorrect results, as nodes may be skipped or duplicated.
 * Unlike linked lists, no _SAFE variant exists.
 *
 * Note also that the macro expands its arguments multiple times, so
 * they should not be expressions with side effects.
 *
 * @param tree A pointer to a struct rbtree to walk
 * @param node The symbol name of a local struct rbnode* variable to
 *             use as the iterator
 */
#define RB_FOR_EACH(tree, node) \
	for (struct _rb_foreach __f = _RB_FOREACH_INIT(tree, node);	\
	     (node = rb_foreach_next(tree, &__f));			\
	     /**/)

/**
 * @brief Loop over rbtree with implicit container field logic
 *
 * As for RB_FOR_EACH(), but "node" can have an arbitrary type
 * containing a struct rbnode.
 *
 * @param tree A pointer to a struct rbtree to walk
 * @param node The symbol name of a local iterator
 * @param field The field name of a struct rbnode inside node
 */
#define RB_FOR_EACH_CONTAINER(tree, node, field)		           \
	for (struct _rb_foreach __f = _RB_FOREACH_INIT(tree, node);	   \
			({struct rbnode *n = rb_foreach_next(tree, &__f); \
			 node = n ? CONTAINER_OF(n, __typeof__(*(node)),   \
					 field) : (void *)0; }) != (void *)0;        \
			 /**/)
#endif

#endif /* SYS_RB_H_ */
