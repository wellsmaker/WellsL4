/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Single-linked list implementation
 *
 * Single-linked list implementation using FORCE_INLINE macros/functions.
 * This API is not thread safe, and thus if a list is used across record_threads,
 * calls to functions must be protected with synchronization primitives.
 */

#ifndef SYS_SFLIST_H_
#define SYS_SFLIST_H_

#include <types_def.h>
#include <sys/stdbool.h>
#include <sys/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __LP64__
typedef u64_t unative_t;
#else
typedef u32_t unative_t;
#endif

struct _sfnode {
	unative_t next_and_flags;
};

typedef struct _sfnode sys_sfnode_t;

struct _sflist {
	sys_sfnode_t *head;
	sys_sfnode_t *tail;
};

typedef struct _sflist sys_sflist_t;


/**
 * @brief Provide the primitive to iterate on a list
 * Note: the loop is unsafe and thus __sn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_SFLIST_FOR_EACH_NODE(l, n) {
 *         <user code>
 *     }
 *
 * This and other SYS_SFLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a sys_sflist_t to iterate on
 * @param __sn A sys_sfnode_t pointer to peek each node of the list
 */
#define SYS_SFLIST_FOR_EACH_NODE(__sl, __sn)				\
	GENLIST_FOR_EACH_NODE(sflist, __sl, __sn)

/**
 * @brief Provide the primitive to iterate on a list, from a node in the list
 * Note: the loop is unsafe and thus __sn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_SFLIST_ITERATE_FROM_NODE(l, n) {
 *         <user code>
 *     }
 *
 * Like SYS_SFLIST_FOR_EACH_NODE(), but __dn already contains a node in the list
 * where to start searching for the next entry from. If NULL, it starts from
 * the head.
 *
 * This and other SYS_SFLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a sys_sflist_t to iterate on
 * @param __sn A sys_sfnode_t pointer to peek each node of the list
 *             it contains the starting node, or NULL to start from the head
 */
#define SYS_SFLIST_ITERATE_FROM_NODE(__sl, __sn)				\
	GENLIST_ITERATE_FROM_NODE(sflist, __sl, __sn)

/**
 * @brief Provide the primitive to safely iterate on a list
 * Note: __sn can be removed, it will not break the loop.
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_SFLIST_FOR_EACH_NODE_SAFE(l, n, s) {
 *         <user code>
 *     }
 *
 * This and other SYS_SFLIST_*() macros are not thread safe.
 *
 * @param __sl A pointer on a sys_sflist_t to iterate on
 * @param __sn A sys_sfnode_t pointer to peek each node of the list
 * @param __sns A sys_sfnode_t pointer for the loop to run safely
 */
#define SYS_SFLIST_FOR_EACH_NODE_SAFE(__sl, __sn, __sns)			\
	GENLIST_FOR_EACH_NODE_SAFE(sflist, __sl, __sn, __sns)

/*
 * @brief Provide the primitive to resolve the container of a list node
 * Note: it is safe to use with NULL pointer nodes
 *
 * @param __ln A pointer on a sys_sfnode_t to get its container
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_CONTAINER(__ln, __cn, __n) \
	GENLIST_CONTAINER(__ln, __cn, __n)

/*
 * @brief Provide the primitive to peek container of the list head
 *
 * @param __sl A pointer on a sys_sflist_t to peek
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_PEEHEAD_CONTAINER(__sl, __cn, __n) \
	GENLIST_PEEHEAD_CONTAINER(sflist, __sl, __cn, __n)

/*
 * @brief Provide the primitive to peek container of the list tail
 *
 * @param __sl A pointer on a sys_sflist_t to peek
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_PEETAIL_CONTAINER(__sl, __cn, __n) \
	GENLIST_PEETAIL_CONTAINER(sflist, __sl, __cn, __n)

/*
 * @brief Provide the primitive to peek the next container
 *
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_PEENEXT_CONTAINER(__cn, __n) \
	GENLIST_PEENEXT_CONTAINER(sflist, __cn, __n)

/**
 * @brief Provide the primitive to iterate on a list under a container
 * Note: the loop is unsafe and thus __cn should not be detached
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_SFLIST_FOR_EACH_CONTAINER(l, c, n) {
 *         <user code>
 *     }
 *
 * @param __sl A pointer on a sys_sflist_t to iterate on
 * @param __cn A pointer to peek each entry of the list
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_FOR_EACH_CONTAINER(__sl, __cn, __n)			\
	GENLIST_FOR_EACH_CONTAINER(sflist, __sl, __cn, __n)

/**
 * @brief Provide the primitive to safely iterate on a list under a container
 * Note: __cn can be detached, it will not break the loop.
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_SFLIST_FOR_EACH_NODE_SAFE(l, c, cn, n) {
 *         <user code>
 *     }
 *
 * @param __sl A pointer on a sys_sflist_t to iterate on
 * @param __cn A pointer to peek each entry of the list
 * @param __cns A pointer for the loop to run safely
 * @param __n The field name of sys_sfnode_t within the container struct
 */
#define SYS_SFLIST_FOR_EACH_CONTAINER_SAFE(__sl, __cn, __cns, __n)	\
	GENLIST_FOR_EACH_CONTAINER_SAFE(sflist, __sl, __cn, __cns, __n)


/*
 * Required function definitions for the list_gen.h interface
 *
 * These are the only functions that do not treat the list/node pointers
 * as completely opaque types.
 */

/**
 * @brief Initialize a list
 *
 * @param list A pointer on the list to initialize
 */
static FORCE_INLINE void sys_sflist_init(sys_sflist_t *list)
{
	list->head = NULL;
	list->tail = NULL;
}

#define SYS_SFLIST_STATIC_INIT(ptr_to_list) {NULL, NULL}
#define SYS_SFLIST_FLAGS_MASK	0x3UL

static FORCE_INLINE sys_sfnode_t *sfnode_next_peek(sys_sfnode_t *node)
{
	return (sys_sfnode_t *)(node->next_and_flags & ~SYS_SFLIST_FLAGS_MASK);
}

static FORCE_INLINE byte_t sys_sfnode_flags_get(sys_sfnode_t *node);

static FORCE_INLINE void sfnode_next_set(sys_sfnode_t *parent,
				       sys_sfnode_t *child)
{
	byte_t cur_flags = sys_sfnode_flags_get(parent);

	parent->next_and_flags = cur_flags | (unative_t)child;
}

static FORCE_INLINE void sflist_head_set(sys_sflist_t *list, sys_sfnode_t *node)
{
	list->head = node;
}

static FORCE_INLINE void sflist_tail_set(sys_sflist_t *list, sys_sfnode_t *node)
{
	list->tail = node;
}

/**
 * @brief Peek the first node from the list
 *
 * @param list A point on the list to peek the first node from
 *
 * @return A pointer on the first node of the list (or NULL if none)
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_peek_head(sys_sflist_t *list)
{
	return list->head;
}

/**
 * @brief Peek the last node from the list
 *
 * @param list A point on the list to peek the last node from
 *
 * @return A pointer on the last node of the list (or NULL if none)
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_peek_tail(sys_sflist_t *list)
{
	return list->tail;
}

/*
 * APIs specific to sflist type
 */

/**
 * @brief Fetch flag value for a particular sfnode
 *
 * @param node A pointer to the node to fetch flag from
 * @return The value of flag, which will be between 0 and 3
 */
static FORCE_INLINE byte_t sys_sfnode_flags_get(sys_sfnode_t *node)
{
	return node->next_and_flags & SYS_SFLIST_FLAGS_MASK;
}

/**
 * @brief Initialize an sflist node
 *
 * Set an initial flag value for this slist node, which can be a value between
 * 0 and 3. These flag will persist even if the node is moved around
 * within a list, removed, or transplanted to a different slist.
 *
 * This is ever so slightly faster than sys_sfnode_flags_set() and should
 * only be used on a node that hasn't been added to any list.
 *
 * @param node A pointer to the node to set the flag on
 * @param flag A value between 0 and 3 to set the flag value
 */
static FORCE_INLINE void sys_sfnode_init(sys_sfnode_t *node, byte_t flag)
{
	assert_info((flag & ~SYS_SFLIST_FLAGS_MASK) == 0UL, "flag too large");
	node->next_and_flags = flag;
}

/**
 * @brief Set flag value for an sflist node
 *
 * Set a flag value for this slist node, which can be a value between
 * 0 and 3. These flag will persist even if the node is moved around
 * within a list, removed, or transplanted to a different slist.
 *
 * @param node A pointer to the node to set the flag on
 * @param flag A value between 0 and 3 to set the flag value
 */
static FORCE_INLINE void sys_sfnode_flags_set(sys_sfnode_t *node, byte_t flag)
{
	assert_info((flag & ~SYS_SFLIST_FLAGS_MASK) == 0UL, "flag too large");
	node->next_and_flags = (unative_t)(sfnode_next_peek(node)) | flag;
}

/*
 * Derived, generated APIs
 */

/**
 * @brief Test if the given list is empty
 *
 * @param list A pointer on the list to test
 *
 * @return a boolean, true if it's empty, false otherwise
 */
static FORCE_INLINE bool sys_sflist_is_empty(sys_sflist_t *list);

GENLIST_IS_EMPTY(sflist)

/**
 * @brief Peek the next node from _current_thread node, node is not NULL
 *
 * Faster then sys_sflist_peek_next() if node is known not to be NULL.
 *
 * @param node A pointer on the node where to peek the next node
 *
 * @return a pointer on the next node (or NULL if none)
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_peek_next_no_check(sys_sfnode_t *node);

GENLIST_PEENEXT_NO_CHECK(sflist, sfnode)

/**
 * @brief Peek the next node from _current_thread node
 *
 * @param node A pointer on the node where to peek the next node
 *
 * @return a pointer on the next node (or NULL if none)
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_peek_next(sys_sfnode_t *node);

GENLIST_PEENEXT(sflist, sfnode)

/**
 * @brief Prepend a node to the given list
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to prepend
 */
static FORCE_INLINE void sys_sflist_prepend(sys_sflist_t *list,
				      sys_sfnode_t *node);

GENLIST_PREPEND(sflist, sfnode)

/**
 * @brief Append a node to the given list
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to append
 */
static FORCE_INLINE void sys_sflist_append(sys_sflist_t *list,
				     sys_sfnode_t *node);

GENLIST_APPEND(sflist, sfnode)

/**
 * @brief Append a list to the given list
 *
 * Append a singly-linked, NULL-terminated list consisting of nodes containing
 * the pointer to the next node as the first element of a node, to @a list.
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * FIXME: Why are the element parameters void *?
 *
 * @param list A pointer on the list to affect
 * @param head A pointer to the first element of the list to append
 * @param tail A pointer to the last element of the list to append
 */
static FORCE_INLINE void sys_sflist_append_list(sys_sflist_t *list,
					  void *head, void *tail);

GENLIST_APPEND_LIST(sflist, sfnode)

/**
 * @brief merge two sflists, appending the second one to the first
 *
 * When the operation is completed, the appending list is empty.
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param list_to_append A pointer to the list to append.
 */
static FORCE_INLINE void sys_sflist_merge_sflist(sys_sflist_t *list,
					   sys_sflist_t *list_to_append);

GENLIST_MERGE_LIST(sflist, sfnode)

/**
 * @brief Insert a node to the given list
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param prev A pointer on the previous node
 * @param node A pointer on the node to insert
 */
static FORCE_INLINE void sys_sflist_insert(sys_sflist_t *list,
				     sys_sfnode_t *prev,
				     sys_sfnode_t *node);

GENLIST_INSERT(sflist, sfnode)

/**
 * @brief Fetch and remove the first node of the given list
 *
 * List must be known to be non-empty.
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 *
 * @return A pointer to the first node of the list
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_get_not_empty(sys_sflist_t *list);

GENLIST_GET_NOT_EMPTY(sflist, sfnode)

/**
 * @brief Fetch and remove the first node of the given list
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 *
 * @return A pointer to the first node of the list (or NULL if empty)
 */
static FORCE_INLINE sys_sfnode_t *sys_sflist_get(sys_sflist_t *list);

GENLIST_GET(sflist, sfnode)

/**
 * @brief Remove a node
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param prev_node A pointer on the previous node
 *        (can be NULL, which means the node is the list's head)
 * @param node A pointer on the node to remove
 */
static FORCE_INLINE void sys_sflist_remove(sys_sflist_t *list,
				     sys_sfnode_t *prev_node,
				     sys_sfnode_t *node);

GENLIST_REMOVE(sflist, sfnode)

/**
 * @brief Find and remove a node from a list
 *
 * This and other sys_sflist_*() functions are not thread safe.
 *
 * @param list A pointer on the list to affect
 * @param node A pointer on the node to remove from the list
 *
 * @return true if node was removed
 */
static FORCE_INLINE bool sys_sflist_find_and_remove(sys_sflist_t *list,
					      sys_sfnode_t *node);

GENLIST_FIND_AND_REMOVE(sflist, sfnode)

#ifdef __cplusplus
}
#endif

#endif /* SYS_SFLIST_H_ */
