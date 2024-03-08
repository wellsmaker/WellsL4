#ifndef L4_SPACE_H_
#define L4_SPACE_H_

#include <inc_l4/types.h>
#include <syscalls/untyped.h>
#include <syscalls/anode.h>
#include <syscalls/cnode.h>

enum L4_SpaceRight {
	L4_Readable			 = 0x04,
	L4_Writable			 = 0x02,
	L4_Xecutable		 = 0x01,
	L4_FullyAccessible	 = 0x07,
	L4_ReadWriteOnly	 = 0x06,
	L4_ReadeXecOnly		 = 0x05,
	L4_NoAccess			 = 0x00,
};

static inline L4_Bool_t L4_IsNilFpage(L4_Fpage_t f)
{
  	return f.raw == 0;
}

static inline L4_Word_t L4_GetRights(L4_Fpage_t f)
{
  	return f.X.rwx;
}

static inline L4_Fpage_t L4_SetRights(L4_Fpage_t *f, L4_Word_t rwx)
{
	f->X.rwx = rwx;
	return *f;
}

static inline L4_Fpage_t L4_FpageAddRights(L4_Fpage_t f, L4_Word_t rwx)
{
	f.X.rwx |= rwx;
	return f;
}

static inline L4_Fpage_t L4_FpageAddRightsTo(L4_Fpage_t * f, L4_Word_t rwx)
{
	f->X.rwx |= rwx;
	return *f;
}

static inline L4_Fpage_t L4_FpageRemoveRights(L4_Fpage_t f, L4_Word_t rwx)
{
	f.X.rwx &= ~rwx;
	return f;
}

static inline L4_Fpage_t L4_FpageRemoveRightsFrom(L4_Fpage_t *f, L4_Word_t rwx)
{
	f->X.rwx &= ~rwx;
	return *f;
}

static inline L4_Fpage_t L4_Fpage(L4_Word_t BaseAddress, L4_Word_t FpageSize)
{
	L4_Fpage_t fp;
	
	L4_Word_t msb = L4_Msb(FpageSize);
	fp.raw = BaseAddress;
	fp.X.s = (1UL << msb) < FpageSize ? msb + 1 : msb;
	fp.X.rwx = L4_NoAccess;
	
	return fp;
}

static inline L4_Fpage_t L4_FpageLog2(L4_Word_t BaseAddress, sword_t FpageSize)
{
	L4_Fpage_t fp;
	
	fp.raw = BaseAddress;
	fp.X.s = FpageSize;
	fp.X.rwx = L4_NoAccess;
	
	return fp;
}

static inline L4_Word_t L4_Address(L4_Fpage_t f)
{
  	return f.raw & ~((1UL << f.X.s) - 1);
}

static inline L4_Word_t L4_Size(L4_Fpage_t f)
{
  	return f.X.s == 0 ? 0 : (1UL << f.X.s);
}

static inline L4_Word_t L4_SizeLog2(L4_Fpage_t f)
{
  	return f.X.s;
}

static inline L4_Bool_t L4_WasWritten(L4_Fpage_t f)
{
	return (f.raw & L4_Writable) != 0;
}

static inline L4_Bool_t L4_WasReferenced(L4_Fpage_t f)
{
  	return (f.raw & L4_Readable) != 0;
}

static inline L4_Bool_t L4_WaseXecuted(L4_Fpage_t f)
{
  	return (f.raw & L4_eXecutable) != 0;
}

/* L4_Unmap : unmap page syscall function
  The specified fpages (located in MR0:::) are unmapped. Fpages are mapped as part of the IPC operation (see page 64).

  [Systemcall]
  output:
	none
  input:
	1. control: 0 f k
  pagefaults:
	none
	
  input registers:
    FpageList MR0...k
  output registers:
    FpageList MR0...k
*/


void L4_Unmap(L4_Word_t control)
{
	L4_Word_t except;
	except = unmap_page(control);
}

static inline L4_Fpage_t L4_UnmapFpage(L4_Fpage_t f)
{
	L4_LoadMR(0, f.raw);
	L4_Unmap((L4_Word_t)0);
	L4_StoreMR(0, &f.raw);

	return f;
}

static inline void L4_UnmapFpages(L4_Word_t n, L4_Fpage_t *fpages)
{
	L4_LoadMRs(0, n, (L4_Word_t *) fpages);
	L4_Unmap(n - 1);
	L4_StoreMRs(0, n, (L4_Word_t *) fpages);
}

static inline L4_Fpage_t L4_Flush(L4_Fpage_t f)
{
	L4_LoadMR(0, f.raw);
	/* f = 1 */
	L4_Unmap(0x40);
	L4_StoreMR(0, &f.raw);

	return f;
}

static inline void L4_FlushFpages(L4_Word_t n, L4_Fpage_t *fpages)
{
	L4_LoadMRs(0, n, (L4_Word_t *) fpages);
	/* f = 1 */
	L4_Unmap(0x40 + n - 1);
	L4_StoreMRs(0, n, (L4_Word_t *) fpages);
}

static inline L4_Fpage_t L4_GetStatus(L4_Fpage_t f)
{
	L4_LoadMR(0, f.raw & ~0x0f);
	L4_Unmap((L4_Word_t) 0);
	L4_StoreMR(0, &f.raw);

	return f;
}

#endif
