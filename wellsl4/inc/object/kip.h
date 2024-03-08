#ifndef KIP_H_
#define KIP_H_

#include <types_def.h>
#include <kernel_object.h>
#include <api/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/*L4 X.2 Reference Manual Rev.7 Standard(DIFF!!!)*/

union kip_apiv {
	struct{
		word_t  version:8;      /* 0x02,0x83,0x84,0x85,0x86,0x04 */
		word_t  subversion:8;   /* 0x80,0x81 */
		word_t  reserved:16;
	}s;
	word_t raw;
};

typedef union kip_apiv kip_apiv_t;

union kip_apif {
	struct{
		word_t  reserved : 28;
		word_t  ww : 2;         /* 00:32bits api; 01:64bits api */
		word_t  ee : 2;		  /* 00:little endian; 01:big endian */
	}s;
	word_t raw;
};

typedef union kip_apif kip_apif_t;

union kip_processor_info {
	struct {
		word_t size       : 4;
		word_t reserved   : 12;
		word_t processors : 16;
	}s;
	word_t raw;
};

typedef union kip_processor_info kip_processor_info_t;

union kip_page_info {
	struct {
		word_t page_size_mask : 22;
		word_t reserved       : 7;
		word_t rwx            : 3;
	}s;
	word_t raw;
};

typedef union kip_page_info kip_page_info_t;


union kip_thread_info {
	struct{
		word_t user_base:12;     /*Lowest thread number available for user threads (see thread_page 14). The first three thread numbers
									 will be used for the initial thread of 0, 1, and root task respectively (see thread_page 92).
									 The version numbers (see thread_page 14) for these initial threads will equal to 1*/
		word_t system_base:12;   /*Lowest thread number used for system threads (see thread_page 14). Thread numbers below this value
									 denote hardware interrupts*/
		word_t thread_num:8;     /* Number of valid thread-number bits */
	}s;
	word_t raw;
};

typedef union kip_thread_info kip_thread_info_t;

union kip_clock_info {
	struct {
		word_t schedule_precision:16;
		word_t read_precision:16;
	}s;
	word_t raw;
};

typedef union kip_clock_info kip_clock_info_t;


union kip_utcb_info {
	struct {
		word_t reserved : 10;
		word_t size     : 6;
		word_t align    : 6;
		word_t mult     : 10;
	}s;
	word_t raw;
};

typedef union kip_utcb_info kip_utcb_info_t;

union kip_area_info {
	struct {
		word_t kip_size : 6;
		word_t reserved : 26;
	}s;
	word_t raw;
};

typedef union kip_area_info kip_area_info_t;

union kip_mem_info {
	struct{
		word_t mem_ptr:16; /*Location of first memory descriptor 
							 (as an offset relative to the kernel-
							 interface page’s base address)*/
		word_t mem_num:16; /*Number of memory descriptors*/
	}s;
	word_t raw;
};

typedef union kip_mem_info kip_mem_info_t;

/*kip memory descriptors - (kip_ptr + mem_ptr) : mem_num */
struct kip_mem {
	word_t 	base;	/* Last 6 bits contains poolid - region map id */
	word_t	size;	/* Last 6 bits contains tag - region map tag */
};

typedef struct kip_mem kip_mem_t;

struct kip_info {
	word_t 			kid;            /* kernel id   */
	kip_apiv_t 		apiv;		    /* API Version */
	kip_apif_t 		apif;		    /* API Flags   */
	word_t 			kptr;           /* KernDescPtr */
	word_t          reserved1[17];
	kip_mem_info_t  mem_info;       /* MemoryInfo  */
	word_t          reserved2[20];
	kip_utcb_info_t utcb_info;		/* utcb info   */
	kip_area_info_t kip_area_info;	/* kip area info */
	word_t 			reserved3[2];
	word_t 			boot_info;		/* boot info */
	word_t 			pptr;       	/* ProcDescPtr */
	
	kip_clock_info_t 	 clock_info;	/* clock info  */
	kip_thread_info_t 	 thread_info;   /* thread info */
	kip_page_info_t      page_info;     /* thread_page info   */
	kip_processor_info_t processor_info;/* processor info */
	
	word_t          syscalls[12];   /* syscall function(include p-syscall) */
};

typedef struct kip_info kip_t;

/* KIP declarations */
extern struct kip_info kip;

/*
__syscall word_t kernel_interface(
	word_t *version, 
	word_t *flag, 
	word_t *id);

*/
#ifdef __cplusplus
}
#endif

#endif
