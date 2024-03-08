#ifndef L4_MEMORY_H_
#define L4_MEMORY_H_

#include <inc_l4/types.h>
#include <syscalls/objecttype.h>

#define L4_NUM_ATTRIBUTE 4

/* L4_MemoryControl : processer configuration syscall function
   Set the page attributes of the fpages (MR 0...k) to the attribute specified with the fpage.

   [Privileged Systemcall]
   output:
	1. result:The result is 1 if the operation succeeded, otherwise the result is 0 and the ErrorCode TCR
		  indicates the failure reason.
  
   input:
	1. control
	2. attribute1
	3. attribute2
	4. attribute3
	5. attribute4
	
   pagefaults:
	none

   ErrorCode:
	1 5
	
   input registers:
	 FpageList MR0...k

*/

L4_Word_t L4_MemoryControl(L4_Word_t control, 
	const L4_Word_t *attributes)
{
	L4_Word_t result;
	
	return result;
}

static inline L4_Word_t L4_SetPageAttribute(L4_Fpage_t f, 
	L4_Word_t attribute)
{
	L4_Word_t attributes[L4_NUM_ATTRIBUTE];

	attributes[0] = attribute;
	L4_SetRights(&f, 0);
	L4_LoadMR(0, f.raw);

	return L4_MemoryControl(0, attributes);
}

static inline L4_Word_t L4_SetPageAttributes(L4_Word_t n, 
	L4_Fpage_t *f,
	const L4_Word_t *attributes)
{
	L4_LoadMRs(0, n, (L4_Word_t *)f);

	return L4_MemoryControl(n - 1, attributes);
}

#endif
