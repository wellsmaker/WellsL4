#ifndef IPC_H_
#define IPC_H_

#include <api/errno.h>
#include <types_def.h>
#include <kernel_object.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ARCH64)
#define PLUS_32 +32
#else
#define PLUS_32
#endif

#define MESSAGE_REGISTER_NUM 16  										
#define STRING_ITEM 	(1UL << 3) 						/*0-TRUE*/
#define MAP_ITEM    	(1UL << 3) 						/*1-TRUE*/
#define GRANT_ITEM  	(1UL << 3 | 1UL << 1) 			/*1-TRUE*/
#define CTRLXFER_ITEM 	(1UL << 3 | 1UL << 2)  			/*1-TRUE*/
#define RESERVED_ITEM 	(1UL << 3 | 1UL << 2 | 1UL << 1) /*1-TRUE*/

enum ipc_flag {
	ipc_success = 0x8,
	ipc_propagte = 0x1,
	ipc_redirecte = 0x2,
	ipc_xcpu = 0x4,
};

enum where_item {
	message_last_item = 0,
	message_next_item
};

enum where_page {
	page_map = 0,
	page_grant,
	page_unmap,
	e_map_action
};

/* Messages And Message Registers (MRs) [Virtual Registers] */
/* MsgTag[MR0] */
typedef union message_tag {
	struct {
		word_t u:6; 		/*number of untyped words:MR1...u;
								u=0 denotes a message without untyped words*/
		word_t t:6; 		/*number of typed-item words:MRu+1...u+t;
								t=0 denotes a message without typed words*/
		word_t flag:4;	/*message flag*/

		word_t label:16 PLUS_32;  /*request type or invoked method*/
	} s;
	word_t raw;
} message_tag_t;

static inline word_t message_get_tag_u(message_tag_t tag)
{
	return tag.s.u;
}

static inline word_t message_get_tag_t(message_tag_t tag)
{
	return tag.s.t;
}

static inline word_t message_get_tag_flag(message_tag_t tag)
{
	return tag.s.flag;
}

static inline word_t message_get_tag_label(message_tag_t tag)
{
	return tag.s.label;
}

static inline word_t message_get_tag(message_tag_t tag)
{
	return tag.raw;
}

/* untyped words[MR1...u] */
/* typed items[MRu+1...u+t] */
typedef union message_gmsc_items {
	struct {
		word_t typed_encode:4;
		word_t zero:6;
		word_t base:22 PLUS_32;
		word_t rights:4;
		word_t thread_page:28 PLUS_32;
	} grant;
	
	struct {
		word_t typed_encode:4;
		word_t zero:6;
		word_t base:22 PLUS_32;
		word_t rights:4;
		word_t thread_page:28 PLUS_32;
	} map;
	
	struct {
		word_t typed_encode:4;
		word_t zero:6;
		word_t length:22 PLUS_32;
		word_t ptr:32 PLUS_32;		
	} string;

	struct {
		word_t typed_encode:4;
		word_t id:8;
		word_t mask:20 PLUS_32;
		word_t reg:32 PLUS_32;
	} ctrl;

	word_t raw[2];
} message_gmsc_items_t;

#define TYPED_ITEM(item) (item.raw)

static inline word_t message_get_encode(message_gmsc_items_t item)
{
	return(item.raw[0] & 0xF);
}

static inline word_t message_get_base(message_gmsc_items_t item)
{
	return(item.raw[0] & ~0x3FF); /* 10 bit */
}

static inline word_t message_get_page(message_gmsc_items_t item)
{
	return((item.raw[1] & ~0xF) >> 4); /* 16 word unit */
}

static inline word_t message_get_right(message_gmsc_items_t item)
{
	return(item.raw[1] & 0xF); /* 0rwx */
}

static inline word_t message_get_length(message_gmsc_items_t item)
{
	return((item.raw[0] & ~0x3FF) >> 10);
}

static inline word_t message_get_address(message_gmsc_items_t item)
{
	return(item.raw[1]);
}

static inline word_t message_get_id(message_gmsc_items_t item)
{
	return((item.raw[0] & 0xFF0) >> 4);
}

static inline word_t message_get_mask(message_gmsc_items_t item)
{
	return((item.raw[0] & ~0xFFF) >> 12);
}

static inline word_t message_get_reg(message_gmsc_items_t item)
{
	return(item.raw[1]);
}

typedef union message_time {
	struct {
		word_t	m : 10;  /*2^e * m (us)*/
		word_t	e : 5;
		word_t	a : 1;   /*0*/
	} period;
	struct {
		word_t	m : 10;  /*2^e * (m + clock_current(us)[e+9 ~ 63] * 2^10) ; if clock_current(us)[e+10] = c*/
		word_t	c : 1;   /*2^e * (m + clock_current(us)[e+9 ~ 63] * 2^10 + 2^10) ; if clock_current(us)[e+10] != c*/
		word_t	e : 4;
		word_t	a : 1;   /*1*/
	} point;
	uint16_t raw;
} message_time_t;

#define TIME_PERIOD_NEVER \
	((message_time_t) { raw : 0 })
#define TIME_PERIOD_ZERO \
	((message_time_t) { period : { 0, 1, 0 }})

static inline word_t message_time_period_m(message_time_t t)
{
	return(t.period.m);
}

static inline word_t message_time_period_e(message_time_t t)
{
	return(t.period.e);
}

static inline word_t message_time_point_m(message_time_t t)
{
	return(t.point.m);
}

static inline word_t message_time_point_e(message_time_t t)
{
	return(t.point.e);
}

static inline word_t message_time_point_c(message_time_t t)
{
	return(t.point.c);
}

static FORCE_INLINE word_t load_message_registers(struct ktcb *from, word_t msg_num)
{
	word_t message_word;
	
	switch (msg_num)
	{
		case 0:
			message_word = from->callee_saved.mr[5];
			break;
		case 1:
			message_word = from->callee_saved.mr[6];
			break;
		case 2:
			message_word = from->callee_saved.mr[7];
			break;
		default:
			message_word = from->user->mr[msg_num - 8];
			break;
	}

	return message_word;
}
static FORCE_INLINE void store_message_registers(struct ktcb *to, word_t msg_num, word_t msg_data)
{
	switch (msg_num)
	{
		case 0:
			to->callee_saved.mr[5] = msg_data;
			break;
		case 1:
			to->callee_saved.mr[6] = msg_data;
			break;
		case 2:
			to->callee_saved.mr[7] = msg_data;
			break;
		default:
			to->user->mr[msg_num - 8] = msg_data;
			break;
	}
}

static FORCE_INLINE void set_message_ipcflag(struct ktcb *thread, word_t flag)
{
	message_tag_t tag;
	
	tag.raw = (word_t)load_message_registers(thread, 0);

	tag.s.flag |= flag;

	store_message_registers(thread, 0, tag.raw);
}

static FORCE_INLINE word_t get_message_ipcflag(struct ktcb *thread)
{
	message_tag_t tag;
	
	tag.raw = (word_t)load_message_registers(thread, 0);
	
	return message_get_tag_flag(tag);
}

void send_ipc(struct ktcb *thread, bool_t blocking, bool_t candonate, message_t *node);
void cancel_ipc(struct ktcb *thread);
void receive_ipc(struct ktcb *thread, bool_t blocking, message_t *node);
void reorder_message_node(struct ktcb *thread);
void reorder_noticenode(struct ktcb *thread);
void complete_signal(struct ktcb *thread, notifation_t *node);
void cancel_signal(struct ktcb *thread, notifation_t *node);
void recevie_signal(struct ktcb *thread, notifation_t *node, bool_t blocking);
void send_signal(notifation_t *node);

/*
__syscall exception_t exchange_ipc(
	word_t recv_gid, 
	word_t send_gid,
	word_t timeout,
	word_t *any_gid)
{
	return EXCEPTION_NONE;

}
*/

exception_t do_exchange_ipc(	
	word_t recv_gid, 
	word_t send_gid,
	word_t timeout,
	word_t *send_any_gid);

#ifdef __cplusplus
}
#endif

#endif
