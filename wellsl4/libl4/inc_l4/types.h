#ifndef L4_TYPES_H_
#define L4_TYPES_H_

#if defined(CONFIG_ARM)
#include <inc_arch/arm/interface.h>
#define CONFIG_32BIT
#define CONFIG_SMALL_ENDIAN
#endif


#if defined(CONFIG_64BIT)
#define plus32 +32
#define pad32  pad32:32
#define th14   32
#define th18   32
#else
#define plus32 +0 
#define pad32  :0
#define th14   14
#define th18   18
#endif

#if defined(CONFIG_BIG_ENDIAN)
#define struct_member2(t,a,b)				t b; t a
#define struct_member3(t,a,b,c)				t c; t b; t a
#define struct_member4(t,a,b,c,d)			t d; t c; t b; t a
#define struct_member5(t,a,b,c,d,e)			t e; t d; t c; t b; t a
#define struct_member6(t,a,b,c,d,e,f)		t f; t e; t d; t c; t b; t a
#define struct_member7(t,a,b,c,d,e,f,g)		t g; t f; t e; t d; t c; t b; t a
#define struct_member8(t,a,b,c,d,e,f,g,h)	t h; t g; t f; t e; t d; t c; t b; t a
#define struct_member9(t,a,b,c,d,e,f,g,h,i)	t i; t h; t g; t f; t e; t d; t c; t b; t a
#define member_order2(a,b)					b,a
#define member_order3(a,b,c)				c,b,a
#define member_order4(a,b,c,d)				d,c,b,a
#define member_order5(a,b,c,d,e)			e,d,c,b,a
#define member_order6(a,b,c,d,e,f)			f,e,d,c,b,a
#define member_order7(a,b,c,d,e,f,g)		g,f,e,d,c,b,a
#else
#define struct_member2(t,a,b)				t a; t b
#define struct_member3(t,a,b,c)				t a; t b; t c
#define struct_member4(t,a,b,c,d)			t a; t b; t c; t d
#define struct_member5(t,a,b,c,d,e)			t a; t b; t c; t d; t e
#define struct_member6(t,a,b,c,d,e,f)		t a; t b; t c; t d; t e; t f
#define struct_member7(t,a,b,c,d,e,f,g)		t a; t b; t c; t d; t e; t f; t g
#define struct_member8(t,a,b,c,d,e,f,g,h)	t a; t b; t c; t d; t e; t f; t g; t h
#define struct_member9(t,a,b,c,d,e,f,g,h,i)	t a; t b; t c; t d; t e; t f; t g; t h; t i
#define member_order2(a,b)					a,b
#define member_order3(a,b,c)				a,b,c
#define member_order4(a,b,c,d)				a,b,c,d
#define member_order5(a,b,c,d,e)			a,b,c,d,e
#define member_order6(a,b,c,d,e,f)			a,b,c,d,e,f
#define member_order7(a,b,c,d,e,f,g)		a,b,c,d,e,f,g
#endif

#if defined(CONFIG_BIG_ENDIAN)
#define MAGIC_NUMBER (('L' << 24) + ('4' << 16) + (230 << 8) + 'K')
#else
#define MAGIC_NUMBER (('K' << 24) + (230 << 16) + ('4' << 8) + 'L')
#endif

#define WORD_SIZE_BITS 	(sizeof(L4_Word_t) * 8)
#define L4_NULL (void *)0

enum _L4_Bool {
	L4_True = 1,
	L4_False = 0,
};

/* thread id */
typedef union {
	L4_Word_t raw;
	struct {
		struct_member2 (
		  	L4_Word_t,
		  	version	: th14,
		 	thread_no : th18
		);
	} X;
} L4_GId_t;

typedef union {
	L4_Word_t raw;
	struct {
		struct_member2 ( 
			L4_Word_t,
			zeros    : 6,
			local_id : 26 plus32
		);
	} X;
} L4_LId_t;

typedef union {
	L4_Word_t raw;
	L4_GId_t  global;
	L4_LId_t  local;
} L4_ThreadId_t;

#define L4_NILTHREAD ((L4_ThreadId_t) { raw : 0UL})
#define L4_ANYTHREAD ((L4_ThreadId_t) { raw : ~0UL})
#define L4_ANYLOCALTHREAD (\
(L4_ThreadId_t) { local : { X : { \
		member_order2 (0, ((1UL << (WORD_SIZE_BITS - 6)) - 1))} } } )

/* thread state */
typedef struct {
  	L4_Word_t raw;
} L4_ThreadState_t;

/* clock */
typedef union {
	L4_U64_t raw;
	struct {
		L4_U32_t low;
		L4_U32_t high;
	} X;
} L4_Clock_t;

/* time */
typedef union {
	L4_U16_t raw;
	struct {
		struct_member3 (
			L4_Word_t,
			m : 10,
			e : 5,
			a : 1
		);
	} period;
	struct {
		struct_member4 (
			L4_Word_t,
			m : 10,
			c : 1,
			e : 4,
			a : 1
		);
	} point;
} L4_Time_t;

#define L4_NEVER ((L4_Time_t) { raw : 0UL })
#define L4_ZEROTIME ((L4_Time_t) { period : { member_order3(0, 1, 0) }})

/* message tag */
typedef union {
	L4_Word_t raw;
	struct {
		struct_member4 (
	    	L4_Word_t,
	    	u	 :6,
	    	t    :6,
	    	flag:4,
	    	label:16 plus32
		);
	} X;
} L4_MsgTag_t;

#define L4_NILTAG ((L4_MsgTag_t) { raw : 0UL })

/* fpage */
typedef union {
	L4_Word_t raw;
	struct {
		struct_member4 ( 
			L4_Word_t,
			rwx 	: 3,
			zero 	: 1,
			s 	: 6,
			b 	: 22 plus32
		);
	} X;
} L4_Fpage_t;

#define L4_NILPAGE		((L4_Fpage_t) { raw : 0UL })
#define L4_COMPLETEAS	((L4_Fpage_t) { X 	: { member_order4(0, 0, 1, 0) }})

#endif
