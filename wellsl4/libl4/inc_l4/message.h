#ifndef L4_MESSAGE_H_
#define L4_MESSAGE_H_

#include <inc_l4/types.h>

typedef union {
	L4_Word_t  raw[2];
	struct {
	    struct_member4 (
			L4_Word_t,
			C       :1,
			type    :3,
			zeros   :6,
			snd_base:22 plus32
		);
		L4_Fpage_t snd_fpage;
	} X;
} L4_MapItem_t;

typedef union {
	L4_Word_t raw[2];
	struct {
	    struct_member4 (
			L4_Word_t,
			C       :1,
			type    :3,
			zeros   :6,
			snd_base:22 plus32
		);
		L4_Fpage_t	snd_fpage;
	} X;
} L4_GrantItem_t;

typedef union {
	L4_Word_t raw[2];
	struct {
		struct_member5 (
			L4_Word_t,
			C		     :1,
			type	     :3,
			j	         :5,
			c            :1,
			string_length:22 plus32
		);
		union {
			void *string_ptr;
			void *substring_ptr[0];
		} str;
	} X;
} L4_StringItem_t;

typedef union {
	L4_Word_t raw[0];
	struct {
	    struct_member4 (
	    	L4_Word_t,
	    	C       :1,
	    	type    :3,
	    	id      :8,
	    	mask    :20 plus32
		);
	} X;
} L4_CtrlXferItem_t;

typedef struct {
  	L4_Word_t raw;
} L4_CacheAllocationHint_t;
#define L4_UseDefaultCacheAllocation ((L4_CacheAllocationHint_t) { raw: 0 })

typedef union {
	L4_Word_t   raw [L4_NUM_MRS];
	L4_Word_t   msg [L4_NUM_MRS];
	L4_MsgTag_t tag;
} L4_Msg_t;

typedef union {
	L4_Word_t raw;
	struct {
		struct_member4 (
			L4_Word_t,
			s        :1,
			c        :1,
			zeros    :2,
			RcvWindow:28 plus32
		);
	} X;
} L4_Acceptor_t;

typedef union    {
	L4_Word_t raw[L4_NUM_BRS - 1];
	L4_StringItem_t	string[0];
} L4_MsgBuffer_t;

#define L4_UntypedWordsAcceptor		((L4_Acceptor_t) { raw: 0 })
#define L4_StringItemsAcceptor		((L4_Acceptor_t) { raw: 1 })
#define L4_CtrlXferAcceptor		((L4_Acceptor_t) { raw: 2 })


/* message tag */
static inline L4_Bool_t L4_IsMsgTagEqual(const L4_MsgTag_t l, 
	const L4_MsgTag_t r)
{
  	return l.raw == r.raw;
}

static inline L4_Bool_t L4_IsMsgTagNotEqual(const L4_MsgTag_t l, 
	const L4_MsgTag_t r)
{
  	return l.raw != r.raw;
}

static inline L4_MsgTag_t L4_MsgTagAddLabel(const L4_MsgTag_t t,
	L4_SWord_t label)
{
	L4_MsgTag_t tag = t;
	tag.X.label = label;
	return tag;
}

static inline L4_MsgTag_t L4_MsgTagAddLabelTo(L4_MsgTag_t *t, 
	L4_SWord_t label)
{
	t->X.label = label;
	return *t;
}

static inline L4_Word_t L4_Label(L4_MsgTag_t t)
{
  	return t.X.label;
}

static inline L4_Word_t L4_UntypedWords(L4_MsgTag_t t)
{
  	return t.X.u;
}

static inline L4_Word_t L4_TypedWords(L4_MsgTag_t t)
{
  	return t.X.t;
}

static inline void L4_Set_Label(L4_MsgTag_t *t, 
	L4_Word_t label)
{
  	t->X.label = label;
}

static inline L4_MsgTag_t L4_Get_MsgTag(void)
{
	L4_MsgTag_t msgtag;
	L4_StoreMR(0, &msgtag.raw);
	return msgtag;
}

static inline void L4_Set_MsgTag(L4_MsgTag_t t)
{
  	L4_LoadMR(0, t.raw);
}

/* map item */
static inline L4_MapItem_t L4_MapItem(L4_Fpage_t f, 
	L4_Word_t SndBase)
{
	L4_MapItem_t item;

	item.raw[0] 			=  	0;
	item.X.type 			=  	0x04;
	item.X.snd_base 		=  	SndBase >> 10;
	item.X.snd_fpage.raw  =  	f.raw;

	return item;
}

static inline L4_Bool_t L4_IsMapItem(L4_MapItem_t m)
{
  	return m.X.type == 0x04;
}

static inline L4_Fpage_t L4_MapItemSndFpage(L4_MapItem_t m)
{
  	return m.X.snd_fpage;
}

static inline L4_Word_t L4_MapItemSndBase(L4_MapItem_t m)
{
  	return m.X.snd_base << 10;
}

/* grant item */
static inline L4_Bool_t L4_IsGrantItem(L4_GrantItem_t g)
{
  	return g.X.type == 0x05;
}

static inline L4_GrantItem_t L4_GrantItem(L4_Fpage_t f, 
	L4_Word_t SndBase)
{
	L4_GrantItem_t item;

	item.raw[0] 			= 	0;
	item.X.type 			= 	0x05;
	item.X.snd_base 		= 	SndBase >> 10;
	item.X.snd_fpage.raw 	= 	f.raw;

	return item;
}

static inline L4_Fpage_t L4_GrantItemSndFpage(L4_GrantItem_t m)
{
  	return m.X.snd_fpage;
}

static inline L4_Word_t L4_GrantItemSndBase(L4_GrantItem_t m)
{
  	return m.X.snd_base << 10;
}

/* string item */
static inline L4_StringItem_t L4_StringItem(L4_SWord_t size, 
	void *address)
{
	L4_StringItem_t item;
	item.X.C = 0;
	item.X.type = 0;
	item.X.j = 0;
	item.X.c = 0;
	item.X.string_length = size;
	item.X.str.string_ptr = address;
	return item;
}

static inline L4_Bool_t L4_IsStringItem(L4_StringItem_t *s)
{
  	return (s->X.type & 0x04) == 0;
}

static inline L4_Bool_t L4_CompoundString(L4_StringItem_t *s)
{
  	return s->X.c != 0;
}

static inline L4_Word_t L4_Substrings(L4_StringItem_t *s)
{
  	return s->X.j + 1;
}

static inline void * L4_Substring(L4_StringItem_t *s, 
	L4_Word_t n)
{
  	return s->X.str.substring_ptr[n - 1];
}

static inline L4_StringItem_t *L4_EndOfString(L4_StringItem_t *s,
	L4_StringItem_t **p)
{
	L4_StringItem_t *prev;

	do {
		prev = s;
		s    = (L4_StringItem_t*) &s->X.str.substring_ptr[s->X.j + 1];
	} while (prev->X.c);

	if (p)
		*p = prev;

	return s;
}

static inline L4_StringItem_t *L4_EndOfStrings(L4_StringItem_t *s,
	L4_StringItem_t **p)
{
	L4_StringItem_t *prev;
	do 
	{
		prev = s;
		s = L4_EndOfString(s, (L4_StringItem_t **) 0);
	} while (prev->X.C);

	if (p)
		*p = prev;

	return s;
}

static inline void L4_CopyString(L4_StringItem_t *d, 
	L4_StringItem_t *s)
{
	L4_Word_t *dest, *from, *end;

	from = (L4_Word_t *) s;
	dest = (L4_Word_t *) d;
	end  = (L4_Word_t *) L4_EndOfString(s, (L4_StringItem_t **) 0);

	while (from < end)
		*dest++ = *from++;
}

static inline L4_StringItem_t *L4_AddSubstringTo(L4_StringItem_t *dest, 
	L4_StringItem_t *substr)
{
	L4_StringItem_t *prev;

	dest      = L4_EndOfString(dest, &prev);
	prev->X.c = 1;

	L4_CopyString(dest, substr);

	return dest;
}

static inline L4_StringItem_t *L4_AddSubstringAddressTo(L4_StringItem_t *dest, 
	void *substring_addr)
{
	L4_StringItem_t *last;

	L4_EndOfString(dest, &last);

	last->X.j++;
	last->X.str.substring_ptr[last->X.j] = substring_addr;

	return dest;
}

/* ctrl item */
static inline L4_Bool_t L4_IsCtrlXferItem(L4_CtrlXferItem_t *s)
{
  	return (s->X.type == 6);
}

static inline void L4_CtrlXferItemInit (L4_CtrlXferItem_t *c, 
	L4_Word_t id)
{
	c->raw[0] = 0;
	c->X.type = 0x06;
	c->X.id = id;
	c->X.mask = 0;
}

static inline void L4_FaultConfCtrlXferItemInit(L4_CtrlXferItem_t *c,
	L4_Word_t fault_id,
	L4_Word_t fault_mask)
{
	c->raw[0] = 0;
	c->X.type = 0x06;
	c->X.id = fault_id;
	c->X.mask = fault_mask;
}

/* cache item */
static inline L4_CacheAllocationHint_t L4_CacheAlloctionHint(const L4_StringItem_t s)
{
	L4_CacheAllocationHint_t hint;
	hint.raw = s.raw[0] & 0x6;
	return hint;
}

static inline L4_Bool_t L4_IsCacheAllocationHintEqual(
	const L4_CacheAllocationHint_t l, 
	const L4_CacheAllocationHint_t r)
{
  	return (l.raw & 0x6) == (r.raw & 0x6);
}

static inline L4_Bool_t L4_IsCacheAllocationHintNotEqual(
	const L4_CacheAllocationHint_t l,
	const L4_CacheAllocationHint_t r)
{
  	return (l.raw & 0x6) != (r.raw & 0x6);
}

static inline L4_StringItem_t L4_AddCacheAllocationHint(
	const L4_StringItem_t dest, 
	const L4_CacheAllocationHint_t h)
{
	L4_StringItem_t item = dest;
	item.raw[0] |= (h.raw & 0x6);
	return item;
}

static inline L4_StringItem_t L4_AddCacheAllocationHintTo(
	L4_StringItem_t * dest, 
	const L4_CacheAllocationHint_t h)
{
	dest->raw[0] |= (h.raw & 0x6);
	return *dest;
}

/* message */
static inline void L4_MsgPut(L4_Msg_t *msg, 
	L4_Word_t label,
	L4_SWord_t u, 
	L4_Word_t *Untyped,
	L4_SWord_t t, 
	void *Items)
{
	L4_SWord_t i;

	for (i = 0; i < u; i++)
		msg->msg[i+1] = Untyped[i];

	for (i = 0; i < t; i++)
		msg->msg[i+u+1] = ((L4_Word_t *) Items)[i];

	msg->tag.X.label = label;
	msg->tag.X.flag = 0;
	msg->tag.X.u = u;
	msg->tag.X.t = t;
}

static inline void L4_MsgGet(const L4_Msg_t *msg, 
	L4_Word_t *Untyped, 
	void *Items)
{
	L4_SWord_t i, u, t;

	u = msg->tag.X.u;
	t = msg->tag.X.t;

	for (i = 0; i < u; i++)
		Untyped[i] = msg->msg[i + 1];

	for (i = 0; i < t; i++)
		((L4_Word_t *) Items)[i] = msg->msg[i + 1 + u];
}

static inline L4_MsgTag_t L4_MsgMsgTag(const L4_Msg_t *msg)
{
  	return msg->tag;
}

static inline void L4_Set_MsgMsgTag(L4_Msg_t *msg, 
	L4_MsgTag_t t)
{
  	msg->tag = t;
}

static inline L4_Word_t L4_MsgLabel(const L4_Msg_t *msg)
{
  	return msg->tag.X.label;
}

static inline void L4_Set_MsgLabel(L4_Msg_t *msg, 
	L4_Word_t label)
{
  	msg->tag.X.label = label;
}

static inline void L4_MsgLoad (L4_Msg_t *msg)
{
  	L4_LoadMRs(0, msg->tag.X.u + msg->tag.X.t + 1, &msg->msg[0]);
}

static inline void L4_MsgStore(L4_MsgTag_t t, 
	L4_Msg_t *msg)
{
	L4_StoreMRs(1, t.X.u + t.X.t, &msg->msg[1]);
	msg->tag = t;
}

static inline void L4_MsgClear(L4_Msg_t *msg)
{
  	msg->msg[0] = 0;
}

static inline void L4_MsgAppendWord(L4_Msg_t *msg, 
	L4_Word_t w)
{
	if (msg->tag.X.t) 
	{
		L4_Word_t i = 1 + msg->tag.X.u + msg->tag.X.t;
		
		for ( ; i > (L4_Word_t)(msg->tag.X.u + 1); i--)
		  msg->msg[i] = msg->msg[i-1];
	}

	msg->msg[++msg->tag.X.u] = w;
}

static inline void L4_MsgAppendMapItem(L4_Msg_t *msg,
	L4_MapItem_t m)
{
	msg->msg[msg->tag.X.u + msg->tag.X.t + 1] = m.raw[0];
	msg->msg[msg->tag.X.u + msg->tag.X.t + 2] = m.raw[1];
	msg->tag.X.t += 2;
}

static inline void L4_MsgAppendGrantItem(L4_Msg_t *msg, 
	L4_GrantItem_t g)
{
	msg->msg[msg->tag.X.u + msg->tag.X.t + 1] = g.raw[0];
	msg->msg[msg->tag.X.u + msg->tag.X.t + 2] = g.raw[1];
	msg->tag.X.t += 2;
}

static inline void L4_MsgAppendSimpleStringItem(L4_Msg_t *msg,
	L4_StringItem_t s)
{
	s.X.c = 0;
	s.X.j = 0;
	msg->msg[msg->tag.X.u + msg->tag.X.t + 1] = s.raw[0];
	msg->msg[msg->tag.X.u + msg->tag.X.t + 2] = s.raw[1];
	msg->tag.X.t += 2;
}

static inline void L4_MsgAppendStringItem(L4_Msg_t *msg, 
	L4_StringItem_t *s)
{
	L4_StringItem_t *d = (L4_StringItem_t *)&msg->msg[msg->tag.X.u + msg->tag.X.t + 1];
	L4_StringItem_t *e = L4_EndOfString(s, (L4_StringItem_t **) 0);
	L4_SWord_t size    = ((L4_Word_t) e - (L4_Word_t) s) / sizeof (L4_Word_t);

	L4_CopyString(d, s);

	msg->tag.X.t      += size;
}

static inline void L4_MsgAppendCtrlXferItem(L4_Msg_t *msg,
	L4_CtrlXferItem_t *c)
{
	L4_Word_t reg = 0, num = 0, mask = c->X.mask;

	/* Add regs according to mask */
	for (reg += L4_Lsb(mask); mask != 0; mask >>= L4_Lsb(mask) + 1, reg += L4_Lsb(mask) + 1, num++)
		msg->msg[msg->tag.X.u + msg->tag.X.t + 2 + num] = c->raw[reg + 1];

	/* Add item */
	if (num) 
	{
		msg->msg[msg->tag.X.u + msg->tag.X.t + 1] = c->raw[0];
		msg->tag.X.t += 1 + num;
	}
}

static inline void L4_AppendFaultConfCtrlXferItems(
	L4_Msg_t *msg,
	L4_U64_t fault_id_mask,
	L4_Word_t fault_mask,
	L4_Word_t C)
{
	L4_CtrlXferItem_t item;

	L4_Word_t fault_id           = 0;
	L4_Word_t fault_id_mask_low  = fault_id_mask;
	L4_Word_t fault_id_mask_high = (fault_id_mask >> 32);

	for (fault_id += L4_Lsb(fault_id_mask_low); fault_id_mask_low != 0;
			fault_id_mask_low >>= L4_Lsb(fault_id_mask_low) + 1,
			fault_id += L4_Lsb(fault_id_mask_low) + 1) 
	{
		L4_FaultConfCtrlXferItemInit(&item, fault_id, fault_mask);
		item.X.C = 1;
		L4_MsgAppendWord(msg, item.raw[0]);
	}

	fault_id = 32;
	for (fault_id += L4_Lsb(fault_id_mask_high); fault_id_mask_high != 0;
			fault_id_mask_high >>= L4_Lsb(fault_id_mask_high) + 1,
			fault_id += L4_Lsb(fault_id_mask_high) + 1) 
	{
		L4_FaultConfCtrlXferItemInit(&item, fault_id, fault_mask);
		item.X.C = 1;
		L4_MsgAppendWord(msg, item.raw[0]);
	}

	item.X.C = C;
	msg->msg[msg->tag.X.u + msg->tag.X.t] = item.raw[0];
}

static inline void L4_MsgPutWord(L4_Msg_t *msg,
	L4_Word_t u, 
	L4_Word_t w)
{
 	msg->msg[u + 1] = w;
}

static inline void L4_MsgPutMapItem (L4_Msg_t *msg,
	L4_Word_t t, 
	L4_MapItem_t m)
{
	msg->msg[msg->tag.X.u + t + 1] = m.raw[0];
	msg->msg[msg->tag.X.u + t + 2] = m.raw[1];
}

static inline void L4_MsgPutGrantItem(L4_Msg_t *msg, 
	L4_Word_t t, 
	L4_GrantItem_t g)
{
	msg->msg[msg->tag.X.u + t + 1] = g.raw[0];
	msg->msg[msg->tag.X.u + t + 2] = g.raw[1];
}

static inline void L4_MsgPutSimpleStringItem(L4_Msg_t *msg,
	L4_Word_t t, 
	L4_StringItem_t s)
{
	s.X.c = 0;
	s.X.j = 0;
	msg->msg[msg->tag.X.u + t + 1] = s.raw[0];
	msg->msg[msg->tag.X.u + t + 2] = s.raw[1];
}

static inline void L4_MsgPutStringItem(L4_Msg_t *msg, 
	L4_Word_t t, 
	L4_StringItem_t * s)
{
	L4_StringItem_t *d = (L4_StringItem_t *) &msg->msg[msg->tag.X.u + t + 1];

	L4_CopyString(d, s);
}

static inline void L4_MsgPutCtrlXferItem(L4_Msg_t *msg, 
	L4_Word_t t, 
	L4_CtrlXferItem_t *c)
{
	L4_Word_t reg = 0, num = 0, mask = c->X.mask;

	/* Put regs according to mask */
	for (reg += L4_Lsb(mask); mask != 0; mask >>= L4_Lsb(mask) + 1, reg += L4_Lsb(mask) +1, num++)
		msg->msg[msg->tag.X.u + t + 2 + num] = c->raw[reg + 1];

	/* Put item */
	if (num)
		msg->msg[msg->tag.X.u + t + 1] = c->raw[0];

}

static inline L4_Word_t L4_MsgWord(L4_Msg_t *msg, 
	L4_Word_t u)
{
  	return msg->msg[u + 1];
}

static inline void L4_MsgGetWord(L4_Msg_t *msg,
	L4_Word_t u,
	L4_Word_t *w)
{
  	*w = msg->msg[u + 1];
}

static inline L4_Word_t L4_MsgGetMapItem(L4_Msg_t *msg,
	L4_Word_t t,
	L4_MapItem_t *m)
{
	m->raw[0] = msg->msg[msg->tag.X.u + t + 1];
	m->raw[1] = msg->msg[msg->tag.X.u + t + 2];
	return 2;
}

static inline L4_Word_t L4_MsgGetGrantItem(L4_Msg_t *msg, 
	L4_Word_t t, 
	L4_GrantItem_t *g)
{
	g->raw[0] = msg->msg[msg->tag.X.u + t + 1];
	g->raw[1] = msg->msg[msg->tag.X.u + t + 2];
	return 2;
}

static inline L4_Word_t L4_MsgGetStringItem(L4_Msg_t *msg,
	L4_Word_t t, 
	L4_StringItem_t *s)
{
	L4_StringItem_t *b = (L4_StringItem_t *) &msg->msg[msg->tag.X.u + t + 1];
	L4_StringItem_t *e = L4_EndOfString(b, &s);
	L4_CopyString(s, b);

	return ((L4_Word_t) e - (L4_Word_t) b) / sizeof(L4_Word_t);
}

static inline L4_Word_t L4_MsgGetCtrlXferItem (L4_Msg_t *msg, 
	L4_Word_t mr, 
	L4_CtrlXferItem_t *c)
{
	L4_Word_t reg = 0, num = 0, mask = 0;

	/* Store item */
	c->raw[0] = msg->msg[mr];
	mask = c->X.mask;

	/* Store regs according to mask */
	for (reg += L4_Lsb(mask); mask != 0;
		  mask >>= L4_Lsb(mask) + 1,reg += L4_Lsb(mask) + 1, num++)
		c->raw[reg + 1] = msg->msg[mr + 1 + num];

	return num + 1;
}


static inline L4_Acceptor_t L4_MapGrantItems(L4_Fpage_t RcvWindow)
{
	L4_Acceptor_t a;

	a.raw     = RcvWindow.raw;
	a.X.s     = 0;
	a.X.zeros = 0;

	return a;
}

static inline L4_Acceptor_t L4_AddAcceptor(const L4_Acceptor_t l, 
	const L4_Acceptor_t r)
{
	L4_Acceptor_t a;

	a.raw = 0;
	a.X.s = (l.X.s | r.X.s);
	a.X.RcvWindow = (r.X.RcvWindow != 0) ? r.X.RcvWindow : l.X.RcvWindow;

	return a;
}

static inline L4_Acceptor_t L4_AddAcceptorTo(L4_Acceptor_t l, 
	const L4_Acceptor_t r)
{
	l.X.s = (l.X.s | r.X.s);

	if (r.X.RcvWindow != 0)
		l.X.RcvWindow = r.X.RcvWindow;

	return l;
}

static inline L4_Acceptor_t L4_RemoveAcceptor(const L4_Acceptor_t l,
	const L4_Acceptor_t r)
{
	L4_Acceptor_t a = l;

	if (r.X.s)
		a.X.s = 0;
	if (r.X.RcvWindow != 0)
		a.X.RcvWindow = 0;

	return a;
}

static inline L4_Acceptor_t L4_RemoveAcceptorFrom(L4_Acceptor_t l, 
	const L4_Acceptor_t r)
{
	if (r.X.s)
		l.X.s = 0;

	if (r.X.RcvWindow != 0)
		l.X.RcvWindow = 0;

	return l;
}

static inline L4_Bool_t L4_HasStringItems(const L4_Acceptor_t a)
{
  	return (a.raw & 0x01UL) == 1;
}

static inline L4_Bool_t L4_HasMapGrantItems(const L4_Acceptor_t a)
{
  	return (a.raw & ~0x0fUL) != 0;
}

static inline L4_Fpage_t L4_RcvWindow(const L4_Acceptor_t a)
{
	L4_Fpage_t fp;

	fp.raw   = a.raw;
	fp.X.rwx = 0;

	return fp;
}

static inline void L4_Accept(const L4_Acceptor_t a)
{
  	L4_LoadBR (0, a.raw);
}

static inline void L4_AcceptStrings(const L4_Acceptor_t a, 
	L4_MsgBuffer_t *b)
{
	const L4_StringItem_t *prev, *t, *s = 
		(const L4_StringItem_t *) &b->string[0];
	
	L4_SWord_t n, i = 1;

	L4_LoadBR(0, a.raw);
	
	do {
		prev = s;
		do {
			t = s;
			n = s->X.j + 2;
			s = (const L4_StringItem_t *) &s->X.str.substring_ptr[n-1];
			L4_LoadBRs (i, n, (const L4_Word_t *) t);
			i += n;
		} while (t->X.c);	
	} while (prev->X.C);
}

static inline L4_Acceptor_t L4_Accepted(void)
{
	L4_Acceptor_t a;
	
	L4_StoreBR(0, &a.raw);
	
	return a;
}

static inline void L4_MsgBufferClear(L4_MsgBuffer_t *b)
{
  	b->raw[0] = b->raw[2] = 0;
}

static inline void L4_MsgBufferAppendSimpleRcvString(L4_MsgBuffer_t *b, 
	L4_StringItem_t s)
{
	L4_StringItem_t *prev, *tmp;

	s.X.j = 0;
	s.X.c = 0;

	if (b->raw[0] || b->raw[2]) 
	{
		tmp = L4_EndOfStrings((L4_StringItem_t *) &b[0], &prev);
		prev->X.C = 1;
		*tmp = s;
	} 
	else
	{
		b->raw[2] = ~0UL;
		((L4_StringItem_t *) b)[0] = s;
		b->string[0].X.C = 0;
	}
}

static inline void L4_MsgBufferAppendRcvString(L4_MsgBuffer_t *b, 
	L4_StringItem_t *s)
{
	L4_StringItem_t *prev, *tmp;

	if (b->raw[0] || b->raw[2])
	{
		tmp = L4_EndOfStrings((L4_StringItem_t *) &b->raw[0], &prev);
		prev->X.C = 1;
	} 
	else
	{
		tmp = (L4_StringItem_t *) &b->raw[0];
	}

	L4_CopyString(tmp, s);
	tmp->X.C = 0;
}

#endif
