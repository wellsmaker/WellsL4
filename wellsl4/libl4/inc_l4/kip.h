#ifndef L4_KIP_H_
#define L4_KIP_H_

#include <inc_l4/types.h>
#include <syscalls/kip.h>

typedef union {
	L4_Word_t raw;
	struct {
		struct_member4 (
			L4_Word_t,
			padding:16,
			subid  :8,
			id     :8,
			pad32
		);
	} x;
} L4_KernelId_t;

typedef union {
	L4_Word_t raw;
	struct {
		struct_member4 (
			L4_Word_t,
			padding   :16,
			subversion:8,
			version   :8,
			pad32
		);
	} x;
} L4_ApiVersion_t;

typedef union {
	L4_Word_t raw;
	struct {
		struct_member3 (
			L4_Word_t,
			ee     :2,
			ww     :2,
			padding:28 
			plus32
		);
	} x;
} L4_ApiFlags_t;

typedef struct {
	struct_member2 (
		L4_Word_t,
		MemDescPtr	: WORD_SIZE_BITS / 2,
		n			: WORD_SIZE_BITS / 2
	);
} L4_MemoryInfo_t;

typedef struct {
	L4_Word_t 		magic;
	L4_ApiVersion_t	ApiVersion;
	L4_ApiFlags_t 	ApiFlags;
	L4_Word_t 		KernelVerPtr;
	
	/* 0x10 */
	L4_Word_t 		padding10[17];
	L4_MemoryInfo_t	MemoryInfo;
	L4_Word_t 		padding58[2];
	
	/* 0x60 */
	struct {
		L4_Word_t 	low;
		L4_Word_t 	high;
	} MainMem;
	
	/* 0x68 */
	struct {
		L4_Word_t 	low;
		L4_Word_t 	high;
	} ReservedMem[2];
	
	/* 0x78 */
	struct {
		L4_Word_t 	low;
		L4_Word_t	high;
	} DedicatedMem[5];
	
	/* 0xA0 */
	L4_Word_t 		paddingA0[2];
	union {
		L4_Word_t 	raw;
		struct {
			struct_member4 (	
				L4_Word_t,
				m		:10,
				a		:6,
				s		:6,
				padding	:10 
				plus32
			);
		} X;
	} UtcbAreaInfo;

	union {
		L4_Word_t 	raw;
		struct {
			struct_member2 (	
				L4_Word_t,
				s		:6,
				padding	:26 
				plus32
			);
		} X;
	} KipAreaInfo;

	/* 0xB0 */
	L4_Word_t 	paddingB0[2];
	L4_Word_t 	BootInfo;
	L4_Word_t 	ProcDescPtr;
	
	/* 0xC0 */
	union {
		L4_Word_t raw;
		struct {
			struct_member3 (	
				L4_Word_t,
				ReadPrecision	 :16,
				SchedulePrecision:16,
				pad32
			);
		} X;
	} ClockInfo;

	union {
		L4_Word_t 	raw;
		struct {
			struct_member4 (	
				L4_Word_t,
				t			:8,
				SystemBase	:12,
				UserBase	:12,
				pad32
			);
		} X;
	} ThreadInfo;

	union {
		L4_Word_t 	raw;
		struct {
			struct_member3 (	
				L4_Word_t,
				rwx				:3,
				padding			:7,
				page_size_mask	:22 
				plus32
			);
		} X;
	} PageInfo;

	union {
		L4_Word_t 	raw;
		struct {
			struct_member3 (	
				L4_Word_t,
				processors		:16,
				pading			:12 
				plus32,
				s				:4
			);
		} X;
	} ProcessorInfo;

	/* 0xD0 */
	L4_Word_t 	SpaceControl;
	L4_Word_t 	ThreadControl;
	L4_Word_t 	ProcessorControl;
	L4_Word_t 	MemoryControl;
	
	/* 0xE0 */
	L4_Word_t 	Ipc;
	L4_Word_t 	Lipc;
	L4_Word_t 	Unmap;
	L4_Word_t 	ExchangeRegisters;
	
	/* 0xF0 */
	L4_Word_t 	SystemClock;
	L4_Word_t 	ThreadSwitch;
	L4_Word_t 	Schedule;
	L4_Word_t 	paddingF0;
	
	/* 0x100 */
	L4_Word_t 	padding100[4];
	
	/* 0x110 */
	L4_Word_t 	ArchSyscall0;
	L4_Word_t 	ArchSyscall1;
	L4_Word_t 	ArchSyscall2;
	L4_Word_t 	ArchSyscall3;
} L4_KernelInterfacePage_t;

typedef union {
	L4_Word_t 		raw[4];
	
	struct {
		L4_Word_t 	ExternalFreq;
		L4_Word_t 	InternalFreq;
		L4_Word_t 	padding[2];
	} X;
} L4_ProcDesc_t;

typedef struct {
	L4_KernelId_t 	KernelId;
	
	union {
		L4_Word_t 	raw;
		struct {
			struct_member4 (	
				L4_Word_t,
				day			:5,
				month		:4,
				year		:7,
				padding		:16 
				plus32
			);
		} X;
	} KernelGenDate;

	union {
		L4_Word_t 	raw;
		struct {
			struct_member4 (	
				L4_Word_t,
				subsubver	:16,
				subver		:8,
				ver			:8,
				pad32
			);
		} X;
	} KernelVer;
	
	L4_Word_t Supplier;
	L4_U8_t VersionString[0];
} L4_KernelDesc_t;

typedef union {
	L4_Word_t 	raw[2];
	struct {
		struct_member5 (	
			L4_Word_t,
			type		:4,
			t			:4,
			padding1	:1,
			v			:1,
			low			:WORD_SIZE_BITS - 10
		);

		struct_member2 (	
			L4_Word_t,
			padding2	:10,
			high		:WORD_SIZE_BITS - 10
		);
	} x;
} L4_MemoryDesc_t;

/* Values for API version field */
enum L4_ApiVersionStd {
	L4_ApiVersion_2		= 0x02,
	L4_ApiVersion_4		= 0x04,
	L4_ApiVersion_X0	= 0x83,
	L4_ApiVersion_X1	= 0x83,
	L4_ApiVersion_X2	= 0x84,
	L4_ApiVersion_L4Sec	= 0x85,
	L4_ApiVersion_N1	= 0x86,
	L4_ApiSubVersion_X0	= 0x80,
	L4_ApiSubVersion_X1	= 0x81,
	L4_ApiVersion_Okl	= 0xA0,
};

/* Values for API flag field */
enum L4_ApiFlagStd {
	L4_ApiFlag_Le		= 0x00,
	L4_ApiFlag_Be		= 0x01,
	L4_ApiFlag_32Bit	= 0x00,
	L4_ApiFlag_64Bit	= 0x01,
};

/* Values for kernel id/subid field */
enum L4_KidStd {
	L4_KID_L4_486			= (0 << 8) + 1,
	L4_KID_L4_PENTIUM		= (0 << 8) + 2,
	L4_KID_L4_X86			= (0 << 8) + 3,
	L4_KID_L4_MIPS			= (1 << 8) + 1,
	L4_KID_L4_ALPHA			= (2 << 8) + 1,
	L4_KID_FIASCO			= (3 << 8) + 1,
	L4_KID_L4KA_HAZELNUT	= (4 << 8) + 1,
	L4_KID_L4KA_PISTACHIO	= (4 << 8) + 2,
	L4_KID_L4KA_STRAWBERRY	= (4 << 8) + 3,
	L4_KID_NICTA_PISTACHIO	= (5 << 8) + 1,
	L4_KID_AVIC_WELLSL4     = (6 << 8) + 1),
};

/* Values for kernel Supplier field */
#if defined(CONFIG_BIG_ENDIAN)
enum L4_KernelSupplier {
	L4_SUPL_GMD	  	= (('G' << 24) + ('M' << 16) + ('D' << 8) + ' '),
	L4_SUPL_IBM	  	= (('I' << 24) + ('B' << 16) + ('M' << 8) + ' '),
	L4_SUPL_UNSW	= (('U' << 24) + ('N' << 16) + ('S' << 8) + 'W'),
	L4_SUPL_UKA		= (('U' << 24) + ('K' << 16) + ('a' << 8) + ' '),
	L4_SUPL_NICTA	= (('N' << 24) + ('I' << 16) + ('C' << 8) + 'T'),
	L4_SUPL_AVIC    = (('A' << 24) + ('V' << 16) + ('I' << 8) + 'C'),
};
#else
enum L4_KernelSupplier {
	L4_SUPL_GMD		= ((' ' << 24) + ('D' << 16) + ('M' << 8) + 'G'),
	L4_SUPL_IBM		= ((' ' << 24) + ('M' << 16) + ('B' << 8) + 'I'),
	L4_SUPL_UNSW	= (('W' << 24) + ('S' << 16) + ('N' << 8) + 'U'),
	L4_SUPL_UKA		= ((' ' << 24) + ('a' << 16) + ('K' << 8) + 'U'),
	L4_SUPL_NICTA	= (('T' << 24) + ('C' << 16) + ('I' << 8) + 'N'),
	L4_SUPL_AVIC    = (('C' << 24) + ('I' << 16) + ('V' << 8) + 'A'),
};
#endif

enum L4_MemoryType {
	L4_UndefinedMemoryType				= 0x0,
	L4_ConventionalMemoryType			= 0x1,
	L4_ReservedMemoryType				= 0x2,
	L4_DedicatedMemoryType				= 0x3,
	L4_SharedMemoryType					= 0x4,
	L4_BootLoaderSpecificMemoryType		= 0xe,
	L4_ArchitectureSpecificMemoryType	= 0xf,
};

/* L4_KernelInterface : kip syscall function
  Delivers base address of the kernel interface page, API version, and API flags. The latter two values are copies of the
  corresponding fields in the kernel interface page. The API information is delivered in registers through this system call (a)
  to enable unrestricted structural changes of the kernel interface page in future versions, and (b) to enable secure detection
  of the kernel’s endian mode (little/big) and word width (32/64).

  [Slow Systemcall]
  output:
  	1. kernel interface page(return)
  	2. API Version
  	3. API Flags
  	4. Kernel Id
  input:
    none
  pagefaults:
    none
  */

void *L4_KernelInterface(L4_Word_t *api_version, 
	L4_Word_t *api_flags,
	L4_Word_t *kernel_id)
{
	return (void *)kernel_interface(api_version, api_flags, kernel_id);
}

/* L4_KernelInterface derived functions */
static inline void *L4_GetKernelInterface(void)
{
	L4_ApiVersion_t version;
	L4_ApiFlags_t   flag;
	L4_KernelId_t   kernel_id;

	return L4_KernelInterface(&version.raw, &flag.raw, &kernel_id.raw);
}

static inline L4_Word_t L4_ApiVersion(void)
{
	L4_ApiVersion_t version;
	L4_ApiFlags_t   flag;
	L4_KernelId_t   kernel_id;

	L4_KernelInterface(&version.raw, &flag.raw, &kernel_id.raw);
  
  	return version.raw;
}

static inline L4_Word_t L4_ApiFlags(void)
{
	L4_ApiVersion_t version;
	L4_ApiFlags_t   flag;
	L4_KernelId_t   kernel_id;

	L4_KernelInterface(&version.raw, &flag.raw, &kernel_id.raw);
  
  	return flag.raw;
}

static inline L4_Word_t L4_KernelId(void)
{
	L4_ApiVersion_t version;
	L4_ApiFlags_t   flag;
	L4_KernelId_t   kernel_id;

	L4_KernelInterface(&version.raw, &flag.raw, &kernel_id.raw);

	return kernel_id.raw;
}

static inline void L4_KernelGenDate(void *KernelInterface, L4_Word_t *year, L4_Word_t *month, L4_Word_t *day)
{
	L4_KernelInterfacePage_t *kip;
	L4_Word_t gen;

	kip = (L4_KernelInterfacePage_t *)KernelInterface;
	gen = ((L4_Word_t *)((L4_Word_t)KernelInterface + kip->KernelVerPtr))[1];

	*year  = ((gen >> 9) & 0x7f) + 2000;
	*month = ((gen >> 5) & 0x0f);
	*day   = (gen & 0x1f);
}

static inline L4_Word_t L4_KernelVersion(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return ((L4_Word_t *)((L4_Word_t)KernelInterface + kip->KernelVerPtr))[2];
}

static inline L4_Word_t L4_KernelSupplier(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return ((L4_Word_t *)((L4_Word_t)KernelInterface + kip->KernelVerPtr))[3];
}

static inline L4_Word_t L4_NumProcessors(void *KernelInterface)
{
	L4_KernelInterfacePage_t* kip;

	kip = (L4_KernelInterfacePage_t *)KernelInterface;

	return kip->ProcessorInfo.X.processors + 1;
}

static inline L4_Word_t L4_NumMemoryDescriptors (void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->MemoryInfo.n;
}

static inline L4_Word_t L4_PageSizeMask(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->PageInfo.X.page_size_mask << 10;
}

static inline L4_Word_t L4_PageRights(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->PageInfo.X.rwx;
}

static inline L4_Word_t L4_ThreadIdBits(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->ThreadInfo.X.t;
}

static inline L4_Word_t L4_ThreadIdSystemBase(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->ThreadInfo.X.SystemBase;
}

static inline L4_Word_t L4_ThreadIdUserBase(void *KernelInterface)
{
	L4_KernelInterfacePage_t * kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->ThreadInfo.X.UserBase;
}

static inline L4_Word_t L4_ReadPrecision(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->ClockInfo.X.ReadPrecision;
}

static inline L4_Word_t L4_SchedulePrecision(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->ClockInfo.X.SchedulePrecision;
}

static inline L4_Word_t L4_UtcbAreaSizeLog2(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->UtcbAreaInfo.X.s;
}

static inline L4_Word_t L4_UtcbAreaSize(void *KernelInterface)
{
  	return 1UL << L4_UtcbAreaSizeLog2 (KernelInterface);
}

static inline L4_Word_t L4_UtcbAlignmentLog2(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->UtcbAreaInfo.X.a;
}

static inline L4_Word_t L4_UtcbSize(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return (1UL << kip->UtcbAreaInfo.X.a) * kip->UtcbAreaInfo.X.m;
}

static inline L4_Word_t L4_KipAreaSizeLog2(void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->KipAreaInfo.X.s;
}

static inline L4_Word_t L4_KipAreaSize (void *KernelInterface)
{
  	return 1UL << L4_KipAreaSizeLog2(KernelInterface);
}

static inline L4_Word_t L4_BootInfo (void *KernelInterface)
{
	L4_KernelInterfacePage_t *kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return kip->BootInfo;
}

static inline char *L4_KernelVersionString(void *KernelInterface)
{
	L4_KernelInterfacePage_t * kip;

	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	return (string) ((L4_Word_t)KernelInterface + kip->KernelVerPtr + 
			 sizeof (L4_Word_t) * 4);
}

static inline char *L4_Feature(void *KernelInterface, L4_Word_t num)
{
	char *str = L4_KernelVersionString (KernelInterface);

	do
	{
		while(*str++ != 0);
		if (*str == 0)
			return (string)0;
	} 
	while(num--);

	return str;
}

static inline L4_MemoryDesc_t *L4_MemoryDesc(void *KernelInterface, L4_Word_t num)
{
	L4_KernelInterfacePage_t *kip;
	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	if (num >= kip->MemoryInfo.n)
		return (L4_MemoryDesc_t *) 0;

	return (L4_MemoryDesc_t *)((L4_Word_t) kip + kip->MemoryInfo.MemDescPtr) + num;
}

static inline L4_Bool_t L4_IsMemoryDescVirtual(L4_MemoryDesc_t *m)
{
  	return m->x.v;
}

static inline L4_Word_t L4_MemoryDescType(L4_MemoryDesc_t *m)
{
  	return (m->x.type >= 0x0E) ? (m->x.type + (m->x.t << 4)) : (m->x.type);
}

static inline L4_Word_t L4_MemoryDescLow(L4_MemoryDesc_t *m)
{
  	return m->x.low << 10;
}

static inline L4_Word_t L4_MemoryDescHigh(L4_MemoryDesc_t *m)
{
  	return (m->x.high << 10) + 0x3ff;
}

static inline L4_ProcDesc_t *L4_ProcDesc (void *KernelInterface, L4_Word_t cpu)
{
	L4_KernelInterfacePage_t *kip;
	kip = (L4_KernelInterfacePage_t *) KernelInterface;

	if (cpu > kip->ProcessorInfo.X.processors)
		return (L4_ProcDesc_t *) 0;

	return (L4_ProcDesc_t *)((L4_Word_t)kip + kip->ProcDescPtr + 
			((1 << kip->ProcessorInfo.X.s) * cpu));
}

static inline L4_Word_t L4_ProcDescExternalFreq(L4_ProcDesc_t *p)
{
  	return p->X.ExternalFreq;
}

static inline L4_Word_t L4_ProcDescInternalFreq(L4_ProcDesc_t *p)
{
  	return p->X.InternalFreq;
}

#endif
