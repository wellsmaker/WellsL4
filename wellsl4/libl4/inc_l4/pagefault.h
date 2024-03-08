#ifndef L4_PAGEFAULT_H_
#define L4_PAGEFAULT_H_

#include <inc_l4/types.h>
#include <inc_l4/message.h>

typedef union {
	L4_Word_t raw[3];
	struct {
		struct_member5 (
			L4_Word_t,
			u	   :6,		/* 2 */
			t      :6,		/* 0 */
			zero   :4,
			rwx	   :4,
			padding:12 plus32 /* -2 */
		);

		L4_Word_t fault_address;
		L4_Word_t fault_ip;
	} X;
} L4_PfRequest_t;

typedef union {
	L4_Word_t raw[3];
	struct {
		struct_member4 (
			L4_Word_t,
			u	   :6,		/* 0 */
			t      :6,		/* 2 */
			zero   :4,
			padding:16 plus32 /* 0 */
		);
		
		L4_MapItem_t   MapItem;
		L4_GrantItem_t GrantItem;
	} X;
} L4_PfReply_t;

#endif
