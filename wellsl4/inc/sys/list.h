/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYS_LIST_GEN_H_
#define SYS_LIST_GEN_H_

#include <types_def.h>
#include <sys/stdbool.h>
#include <sys/util.h>

#define GENLIST_FOR_EACH_NODE(__lname, __l, __sn)			\
	for (__sn = sys_ ## __lname ## _peek_head(__l); __sn != NULL;	\
	     __sn = sys_ ## __lname ## _peek_next(__sn))


#define GENLIST_ITERATE_FROM_NODE(__lname, __l, __sn)			\
	for (__sn = __sn ? sys_ ## __lname ## _peek_next_no_check(__sn)	\
			 : sys_ ## __lname ## _peek_head(__l);		\
	     __sn != NULL;						\
	     __sn = sys_ ## __lname ## _peek_next(__sn))

#define GENLIST_FOR_EACH_NODE_SAFE(__lname, __l, __sn, __sns)		\
	for (__sn = sys_ ## __lname ## _peek_head(__l),			\
		     __sns = sys_ ## __lname ## _peek_next(__sn);	\
	     __sn != NULL ; __sn = __sns,				\
		     __sns = sys_ ## __lname ## _peek_next(__sn))

#define GENLIST_CONTAINER(__ln, __cn, __n)				\
	((__ln) ? CONTAINER_OF((__ln), __typeof__(*(__cn)), __n) : NULL)

#define GENLIST_PEEHEAD_CONTAINER(__lname, __l, __cn, __n)		\
	GENLIST_CONTAINER(sys_ ## __lname ## _peek_head(__l), __cn, __n)

#define GENLIST_PEETAIL_CONTAINER(__lname, __l, __cn, __n)		\
	GENLIST_CONTAINER(sys_ ## __lname ## _peek_tail(__l), __cn, __n)

#define GENLIST_PEENEXT_CONTAINER(__lname, __cn, __n)		\
	((__cn) ? GENLIST_CONTAINER(					\
			sys_ ## __lname ## _peek_next(&((__cn)->__n)),	\
			__cn, __n) : NULL)

#define GENLIST_FOR_EACH_CONTAINER(__lname, __l, __cn, __n)		\
	for (__cn = GENLIST_PEEHEAD_CONTAINER(__lname, __l, __cn,	\
						  __n);			\
	     __cn != NULL;						\
	     __cn = GENLIST_PEENEXT_CONTAINER(__lname, __cn, __n))

#define GENLIST_FOR_EACH_CONTAINER_SAFE(__lname, __l, __cn, __cns, __n)     \
	for (__cn = GENLIST_PEEHEAD_CONTAINER(__lname, __l, __cn, __n),   \
	     __cns = GENLIST_PEENEXT_CONTAINER(__lname, __cn, __n); \
	     __cn != NULL; __cn = __cns,				\
	     __cns = GENLIST_PEENEXT_CONTAINER(__lname, __cn, __n))

#define GENLIST_IS_EMPTY(__lname)					\
	static FORCE_INLINE bool						\
	sys_ ## __lname ## _is_empty(sys_ ## __lname ## _t *list)	\
	{								\
		return (sys_ ## __lname ## _peek_head(list) == NULL);	\
	}

#define GENLIST_PEENEXT_NO_CHECK(__lname, __nname)			    \
	static FORCE_INLINE sys_ ## __nname ## _t *				    \
	sys_ ## __lname ## _peek_next_no_check(sys_ ## __nname ## _t *node) \
	{								    \
		return  ## __nname ## _next_peek(node);		    \
	}

#define GENLIST_PEENEXT(__lname, __nname)				     \
	static FORCE_INLINE sys_ ## __nname ## _t *				     \
	sys_ ## __lname ## _peek_next(sys_ ## __nname ## _t *node)	     \
	{								     \
		return node != NULL ?                                        \
			sys_ ## __lname ## _peek_next_no_check(node) :       \
			      NULL;					     \
	}

#define GENLIST_PREPEND(__lname, __nname)				      \
	static FORCE_INLINE void						      \
	sys_ ## __lname ## _prepend(sys_ ## __lname ## _t *list,	      \
				    sys_ ## __nname ## _t *node)	      \
	{								      \
		 __nname ## _next_set(node,			      \
					sys_ ## __lname ## _peek_head(list)); \
		 __lname ## _head_set(list, node);			      \
									      \
		if (sys_ ## __lname ## _peek_tail(list) == NULL) {	      \
			 __lname ## _tail_set(list,		      \
					sys_ ## __lname ## _peek_head(list)); \
		}							      \
	}

#define GENLIST_APPEND(__lname, __nname)				\
	static FORCE_INLINE void						\
	sys_ ## __lname ## _append(sys_ ## __lname ## _t *list,		\
				   sys_ ## __nname ## _t *node)		\
	{								\
		 __nname ## _next_set(node, NULL);			\
									\
		if (sys_ ## __lname ## _peek_tail(list) == NULL) {	\
			 __lname ## _tail_set(list, node);		\
			 __lname ## _head_set(list, node);		\
		} else {						\
			 __nname ## _next_set(			\
				sys_ ## __lname ## _peek_tail(list), node);					\
			 __lname ## _tail_set(list, node);		\
		}							\
	}

#define GENLIST_APPEND_LIST(__lname, __nname)				\
	static FORCE_INLINE void						\
	sys_ ## __lname ## _append_list(sys_ ## __lname ## _t *list,	\
					void *head, void *tail)		\
{									\
	if (sys_ ## __lname ## _peek_tail(list) == NULL) {		\
		 __lname ## _head_set(list,			\
					(sys_ ## __nname ## _t *)head); \
	} else {							\
		 __nname ## _next_set(				\
			sys_ ## __lname ## _peek_tail(list),		\
			(sys_ ## __nname ## _t *)head);			\
	}								\
	 __lname ## _tail_set(list,				\
				     (sys_ ## __nname ## _t *)tail);	\
}

#define GENLIST_MERGE_LIST(__lname, __nname)				\
	static FORCE_INLINE void						\
	sys_ ## __lname ## _merge_ ## __lname (				\
				sys_ ## __lname ## _t *list,		\
				sys_ ## __lname ## _t *list_to_append)	\
	{								\
		sys_ ## __nname ## _t *head, *tail;			\
		head = sys_ ## __lname ## _peek_head(list_to_append);	\
		tail = sys_ ## __lname ## _peek_tail(list_to_append);	\
		sys_ ## __lname ## _append_list(list, head, tail);	\
		sys_ ## __lname ## _init(list_to_append);		\
	}

#define GENLIST_INSERT(__lname, __nname)				\
	static FORCE_INLINE void						\
	sys_ ## __lname ## _insert(sys_ ## __lname ## _t *list,		\
				   sys_ ## __nname ## _t *prev,		\
				   sys_ ## __nname ## _t *node)		\
	{								\
		if (prev == NULL) {					\
			sys_ ## __lname ## _prepend(list, node);	\
		} else if ( __nname ## _next_peek(prev) == NULL) {	\
			sys_ ## __lname ## _append(list, node);		\
		} else {						\
			 __nname ## _next_set(node,		\
				 __nname ## _next_peek(prev));	\
			 __nname ## _next_set(prev, node);		\
		}							\
	}

#define GENLIST_GET_NOT_EMPTY(__lname, __nname)			\
	static FORCE_INLINE sys_ ## __nname ## _t *				\
	sys_ ## __lname ## _get_not_empty(sys_ ## __lname ## _t *list)	\
	{								\
		sys_ ## __nname ## _t *node =				\
				sys_ ## __lname ## _peek_head(list);	\
									\
		 __lname ## _head_set(list,			\
				 __nname ## _next_peek(node));	\
		if (sys_ ## __lname ## _peek_tail(list) == node) {	\
			 __lname ## _tail_set(list,		\
				sys_ ## __lname ## _peek_head(list));	\
		}							\
									\
		return node;						\
	}

#define GENLIST_GET(__lname, __nname)					\
	static FORCE_INLINE sys_ ## __nname ## _t *				\
	sys_ ## __lname ## _get(sys_ ## __lname ## _t *list)		\
	{								\
		return sys_ ## __lname ## _is_empty(list) ? NULL :	\
			sys_ ## __lname ## _get_not_empty(list);	\
	}

#define GENLIST_REMOVE(__lname, __nname)				      \
	static FORCE_INLINE void						      \
	sys_ ## __lname ## _remove(sys_ ## __lname ## _t *list,		      \
				   sys_ ## __nname ## _t *prev_node,	      \
				   sys_ ## __nname ## _t *node)		      \
	{								      \
		if (prev_node == NULL) {				      \
			 __lname ## _head_set(list,		      \
				 __nname ## _next_peek(node));	      \
									      \
			/* Was node also the tail? */			      \
			if (sys_ ## __lname ## _peek_tail(list) == node) {    \
				 __lname ## _tail_set(list,	      \
					sys_ ## __lname ## _peek_head(list)); \
			}						      \
		} else {						      \
			 __nname ## _next_set(prev_node,		      \
				 __nname ## _next_peek(node));	      \
									      \
			/* Was node the tail? */			      \
			if (sys_ ## __lname ## _peek_tail(list) == node) {    \
				 __lname ## _tail_set(list,	 prev_node);      \
			}						      \
		}							      \
									      \
		 __nname ## _next_set(node, NULL);			      \
	}

#define GENLIST_FIND_AND_REMOVE(__lname, __nname)			 \
	static FORCE_INLINE bool						 \
	sys_ ## __lname ## _find_and_remove(sys_ ## __lname ## _t *list, \
					    sys_ ## __nname ## _t *node) \
	{								 \
		sys_ ## __nname ## _t *prev = NULL;			 \
		sys_ ## __nname ## _t *test;				 \
									 \
		GENLIST_FOR_EACH_NODE(__lname, list, test) {		 \
			if (test == node) {				 \
				sys_ ## __lname ## _remove(list, prev,	 \
							   node);	 \
				return true;				 \
			}						 \
									 \
			prev = test;					 \
		}							 \
									 \
		return false;						 \
	}

#endif
