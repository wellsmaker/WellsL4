#ifndef L4ARCH_ARM_INTERFACE_H_
#define L4ARCH_ARM_INTERFACE_H_

#include <state/statedata.h>

/* type define */
typedef unsigned long long  L4_U64_t;
typedef unsigned int L4_U32_t;
typedef unsigned short L4_U16_t;
typedef unsigned char L4_U8_t;
typedef unsigned int L4_Word_t;

typedef long long L4_S64_t;
typedef int L4_S32_t;
typedef short L4_S16_t;
typedef char L4_S8_t;
typedef int L4_SWord_t;
typedef L4_Word_t L4_Bool_t;

/* L4.X.2 1.3 */
/* TCR:Thread Control Registers (TCRs), see thread_page 16
  MR:Message Registers (MRs), see thread_page 50
  BR:Buffer Registers (BRs), see thread_page 62 */

#define L4_UTCB() (_current_thread->user)

/* All functions are user-level functions; the microkernel is not involved. */

/* virual register - TCRs */

/* virual register - BRs */
#define L4_NUM_MRS 11

/*
register L4_Word_t L4_MR0 asm("r4"); 0
register L4_Word_t L4_MR1 asm("r5"); 1
register L4_Word_t L4_MR2 asm("r6"); 2
register L4_Word_t L4_MR3 asm("r7"); 3
register L4_Word_t L4_MR4 asm("r8"); 4
register L4_Word_t L4_MR5 asm("r9"); 5
register L4_Word_t L4_MR6 asm("r10"); 6
register L4_Word_t L4_MR7 asm("r11"); 7
*/

register L4_Word_t L4_MR0 asm("r9");
register L4_Word_t L4_MR1 asm("r10");
register L4_Word_t L4_MR2 asm("r11");


static inline L4_Word_t L4_NumMRs(void)
{
	return L4_NUM_MRS;
}

static inline void L4_StoreMR(L4_Word_t i, L4_Word_t *w)
{
	switch (i) 
	{
		case 0: *w = L4_MR0; break;
		case 1: *w = L4_MR1; break;
		case 2: *w = L4_MR2; break;
		/* case 3: *w = L4_MR3; break; */
		/* case 4: *w = L4_MR4; break; */
		/* case 5: *w = L4_MR5; break; */
		/* case 6: *w = L4_MR6; break; */
		/* case 7: *w = L4_MR7; break; */
		default:
			if (i >= 0 && i < L4_NUM_MRS)
				*w = L4_UTCB()->mr[i - 8];
			else
				*w = 0;
	}
}

static inline void L4_LoadMR(L4_Word_t i, L4_Word_t w)
{
	switch (i) 
	{
		case 0: L4_MR0 = w; break;
		case 1: L4_MR1 = w; break;
		case 2: L4_MR2 = w; break;
		/* case 3: L4_MR3 = w; break; */
		/* case 4: L4_MR4 = w; break;
		case 5: L4_MR5 = w; break; */
		/* case 6: L4_MR6 = w; break; */
		/* case 7: L4_MR7 = w; break; */
		default:
			if (i >= 0 && i < L4_NUM_MRS)
				L4_UTCB()->mr[i - 8] = w;
	}
}

static inline void L4_StoreMRs(L4_Word_t i, L4_Word_t k, L4_Word_t *w)
{
	if (i < 0 || k <= 0 || i + k > L4_NUM_MRS)
		return;

	switch (i) 
	{
		case 0: *w++ = L4_MR0; if (--k <= 0) break;
		case 1: *w++ = L4_MR1; if (--k <= 0) break;
		case 2: *w++ = L4_MR2; if (--k <= 0) break;
		/* case 3: *w++ = L4_MR3; if (--k <= 0) break; */
		/* case 4: *w++ = L4_MR4; if (--k <= 0) break;
		case 5: *w++ = L4_MR5; if (--k <= 0) break; */
		/* case 6: *w++ = L4_MR6; if (--k <= 0) break; */
		/* case 7: *w++ = L4_MR7; if (--k <= 0) break; */
		default:
			{
				L4_Word_t *mr = L4_UTCB()->mr;
				
				while (k-- > 0)
					*w++ = *mr++;
			}
	}
}

static inline void L4_LoadMRs(L4_Word_t i, L4_Word_t k, L4_Word_t *w)
{
	if (i < 0 || k <= 0 || i + k > L4_NUM_MRS)
		return;

	switch (i) 
	{
		case 0: L4_MR0 = *w++; if (--k <= 0) break;
		case 1: L4_MR1 = *w++; if (--k <= 0) break;
		case 2: L4_MR2 = *w++; if (--k <= 0) break;
		/* case 3: L4_MR3 = *w++; if (--k <= 0) break; */
		/* case 4: L4_MR4 = *w++; if (--k <= 0) break;
		case 5: L4_MR5 = *w++; if (--k <= 0) break; */
		/* case 6: L4_MR6 = *w++; if (--k <= 0) break; */
		/* case 7: L4_MR7 = *w++; if (--k <= 0) break; */
		default:
			{
				L4_Word_t *mr = L4_UTCB()->mr;
				
				while (k-- > 0)
					*mr++ = *w++;
			}
	}
}

/* virual register - BRs */
#define L4_NUM_BRS 8

static inline L4_Word_t L4_NumBRs(void)
{
	return L4_NUM_BRS;
}

static inline void L4_StoreBR(L4_Word_t i, L4_Word_t *w)
{
	if (i >= 0 && i < L4_NUM_BRS)
		*w = L4_UTCB()->br[i];
}

static inline void L4_LoadBR(L4_Word_t i, L4_Word_t w)
{
	if (i >= 0 && i < L4_NUM_BRS)
		L4_UTCB()->br[i] = w;
}

static inline void L4_StoreBRs(L4_Word_t i, L4_Word_t k, L4_Word_t *w)
{
	L4_Word_t *br;

	if (i < 0 || k <= 0 || i + k > L4_NUM_BRS)
		return;

	br = (L4_Word_t *)(L4_UTCB()->br + i);
	
	while (k-- > 0)
		*w++ = *br++;
}

static inline void L4_LoadBRs(L4_Word_t i, L4_Word_t k, const L4_Word_t *w)
{
	L4_Word_t *br;

	if (i < 0 || k <= 0 || i + k > L4_NUM_BRS)
		return;

	br = (L4_Word_t *)(L4_UTCB()->br + i);
	
	while (k-- > 0)
		*br++ = *w++;
}

/* bit operation */
static inline L4_Word_t L4_Msb(L4_Word_t w)
{
	L4_Word_t zeros;
	
	__asm__ __volatile__
	(
	    "clz %0, %1\n"
	    : "=r"(zeros)
	    : "r"(w)
	);
	
	return 31 - zeros;
}

static inline L4_Word_t L4_Lsb(L4_Word_t w)
{
	L4_Word_t bitnum;
	
	__asm__ __volatile__
	(
	    "bsf %1, %0\n"
	    : "=r"(bitnum)
	    : "rm"(w)
	);
	
	return bitnum;
}

#endif
