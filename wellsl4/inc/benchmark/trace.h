#ifndef BENCHMARK_TRACE_H_
#define BENCHMARK_TRACE_H_

#include <types_def.h>
#include <kernel/time.h>
#include <sys/string.h>
#include <sys/printk.h>
#include <state/statedata.h>


#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0(EVAL0(EVAL0(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL(...)  EVAL4(EVAL4(EVAL4(__VA_ARGS__)))

#define ORDER_MAP_END(...)
#define ORDER_MAP_OUT
#define ORDER_MAP_COMMA ,

#define ORDER_MAP_GET_END2() 0, ORDER_MAP_END
#define ORDER_MAP_GET_END1(...) ORDER_MAP_GET_END2
#define ORDER_MAP_GET_END(...) ORDER_MAP_GET_END1
#define ORDER_MAP_NEXT0(test, next, ...) next ORDER_MAP_OUT
#define ORDER_MAP_NEXT1(test, next) ORDER_MAP_NEXT0(test, next, 0)
#define ORDER_MAP_NEXT(test, next)  ORDER_MAP_NEXT1(ORDER_MAP_GET_END test, next)

#define ORDER_MAP0(f, x, peek, ...) f(x) ORDER_MAP_NEXT(peek, ORDER_MAP1)(f, peek, __VA_ARGS__)
#define ORDER_MAP1(f, x, peek, ...) f(x) ORDER_MAP_NEXT(peek, ORDER_MAP0)(f, peek, __VA_ARGS__)

#define ORDER_MAP_LIST_NEXT1(test, next) ORDER_MAP_NEXT0(test, ORDER_MAP_COMMA next, 0)
#define ORDER_MAP_LIST_NEXT(test, next)  ORDER_MAP_LIST_NEXT1(ORDER_MAP_GET_END test, next)

#define ORDER_MAP_LIST0(f, x, peek, ...) f(x) ORDER_MAP_LIST_NEXT(peek, ORDER_MAP_LIST1)(f, peek, __VA_ARGS__)
#define ORDER_MAP_LIST1(f, x, peek, ...) f(x) ORDER_MAP_LIST_NEXT(peek, ORDER_MAP_LIST0)(f, peek, __VA_ARGS__)

/**
 * Applies the function macro `f` to each of the remaining parameters.
 */
#define ORDER_MAP(f, ...) EVAL(ORDER_MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/**
 * Applies the function macro `f` to each of the remaining parameters and
 * inserts commas between the results.
 */
#define ORDER_MAP_LIST(f, ...) EVAL(ORDER_MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/* Obtain a field's size at compile-time.
 * Internal to this bottom-layer.
 */
#define TRACE_BOTTOM_INTERNAL_FIELD_SIZE(x)      + sizeof(x)

/* Append a field to current event-packet.
 * Internal to this bottom-layer.
 */
#define TRACE_BOTTOM_INTERNAL_FIELD_APPEND(x)		 \
	{						 \
		memcpy(epacket_cursor, &(x), sizeof(x)); \
		epacket_cursor += sizeof(x);		 \
	}

/* Gather fields to a contiguous event-packet, then atomically emit.
 * Used by middle-layer.
 */
#define TRACE_BOTTOM_FIELDS(...)						    \
{									    \
	u8_t epacket[0 ORDER_MAP(TRACE_BOTTOM_INTERNAL_FIELD_SIZE, ##__VA_ARGS__)]; \
	u8_t *epacket_cursor = &epacket[0];				    \
									    \
	ORDER_MAP(TRACE_BOTTOM_INTERNAL_FIELD_APPEND, ##__VA_ARGS__)		    \
	snprintk(epacket, sizeof(epacket), "Track Display: %d", _current_thread->thread_id); \
	printk("Track Display: %d\r\n", _current_thread->thread_id); \
}

/* No need for locking when bottom_emit does POSIX fwrite(3) which is thread
 * safe. Used by middle-layer.
 */
#define TRACE_BOTTOM_LOCK()         { /* empty */ }
#define TRACE_BOTTOM_UNLOCK()       { /* empty */ }

/* Limit strings to 20 bytes to optimize bandwidth */
#define TRACE_MAX_STRING_LEN 20

/* Optionally enter into a critical region, decided by bottom layer */
#define TRACE_CRITICAL_REGION(x)	     \
	{			     \
		TRACE_BOTTOM_LOCK();   \
		x;		     \
		TRACE_BOTTOM_UNLOCK(); \
	}

/* Emit CTF event using the bottom-level IO mechanics. Prefix by sample time */
#define TRACE_EVENT(...)							    \
	{								    \
		const u32_t tick = get_cycle_32();			    \
		TRACE_CRITICAL_REGION(TRACE_BOTTOM_FIELDS(tick, __VA_ARGS__)) \
	}

/* Anonymous compound literal with 1 member. Legal since C99.
 * This permits us to take the address of literals, like so:
 *  &TRACE_LITERAL(int, 1234)
 *
 * This may be required if a bottom layer uses memcpy.
 *
 * NOTE string literals already support address-of and sizeof,
 * so string literals should not be wrapped with TRACE_LITERAL.
 */
#define TRACE_LITERAL(type, value)  ((type) { (type)(value) })

enum {
	TRACE_EVENT_THREAD_SWITCHED_OUT   =  0x10,
	TRACE_EVENT_THREAD_SWITCHED_IN    =  0x11,
	TRACE_EVENT_THREAD_PRIORITY_SET   =  0x12,
	TRACE_EVENT_THREAD_CREATE         =  0x13,
	TRACE_EVENT_THREAD_ABORT          =  0x14,
	TRACE_EVENT_THREAD_SUSPEND        =  0x15,
	TRACE_EVENT_THREAD_RESUME         =  0x16,
	TRACE_EVENT_THREAD_READY          =  0x17,
	TRACE_EVENT_THREAD_PENDING        =  0x18,
	TRACE_EVENT_THREAD_INFO           =  0x19,
	TRACE_EVENT_THREAD_NAME_SET       =  0x1A,
	TRACE_EVENT_ISR_ENTER             =  0x20,
	TRACE_EVENT_ISR_EXIT              =  0x21,
	TRACE_EVENT_ISR_EXIT_TO_SCHEDULER =  0x22,
	TRACE_EVENT_IDLE                  =  0x30,
	TRACE_EVENT_ID_START_CALL         =  0x41,
	TRACE_EVENT_ID_END_CALL           =  0x42
};


typedef struct {
	char buf[TRACE_MAX_STRING_LEN];
} string_t;


static inline void trace_thread_switched_out(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_SWITCHED_OUT),
		thread_id
		);
}

static inline void trace_thread_switched_in(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_SWITCHED_IN),
		thread_id
		);
}

static inline void trace_thread_priority_set(u32_t thread_id, s8_t prio)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_PRIORITY_SET),
		thread_id,
		prio
		);
}

static inline void trace_thread_create(
	u32_t thread_id,
	s8_t prio,
	string_t name
	)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_CREATE),
		thread_id,
		name
		);
}

static inline void trace_thread_abort(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_ABORT),
		thread_id
		);
}

static inline void trace_thread_suspend(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_SUSPEND),
		thread_id
		);
}

static inline void trace_thread_resume(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_RESUME),
		thread_id
		);
}

static inline void trace_thread_ready(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_READY),
		thread_id
		);
}

static inline void trace_thread_pend(u32_t thread_id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_PENDING),
		thread_id
		);
}

static inline void trace_thread_info(
	u32_t thread_id,
	u32_t stack_base,
	u32_t stack_size
	)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_INFO),
		thread_id,
		stack_base,
		stack_size
		);
}

static inline void trace_thread_name_set(
	u32_t thread_id,
	string_t name
	)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_THREAD_NAME_SET),
		thread_id,
		name
		);
}

static inline void trace_isr_enter(void)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_ISR_ENTER)
		);
}

static inline void trace_isr_exit(void)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_ISR_EXIT)
		);
}

static inline void trace_isr_exit_to_scheduler(void)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_ISR_EXIT_TO_SCHEDULER)
		);
}

static inline void trace_idle(void)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_IDLE)
		);
}

static inline void trace_void(u32_t id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_ID_START_CALL),
		id
		);
}

static inline void trace_end_call(u32_t id)
{
	TRACE_EVENT(
		TRACE_LITERAL(u8_t, TRACE_EVENT_ID_END_CALL),
		id
		);
}


void sys_trace_thread_switched_out(void);
void sys_trace_thread_switched_in(void);
void sys_trace_thread_priority_set(struct ktcb *thread);
void sys_trace_thread_create(struct ktcb *thread);
void sys_trace_thread_abort(struct ktcb *thread);
void sys_trace_thread_suspend(struct ktcb *thread);
void sys_trace_thread_resume(struct ktcb *thread);
void sys_trace_thread_ready(struct ktcb *thread);
void sys_trace_thread_pend(struct ktcb *thread);
void sys_trace_thread_info(struct ktcb *thread);
void sys_trace_thread_name_set(struct ktcb *thread);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_isr_exit_to_scheduler(void);
void sys_trace_idle(void);
void sys_trace_void(unsigned int id);
void sys_trace_end_call(unsigned int id);

#endif
