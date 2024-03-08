#ifndef DEVICE_H_
#define DEVICE_H_

#include <types_def.h>

#include <lib/bitmap.h>

struct IO_FILE;

typedef struct IO_FILE FILE;

struct IO_DEVICE_ITEM {
	union {
		FILE *head;
		FILE *prev;
	} u0;
	union {
		FILE *tail;
		FILE *next;
	} u1;
};

struct IO_FILE {  
	sword_t            fd;
	word_t   flag;
	signed char    mode;
	
	size_t         buf_size; /* IO data buffer size(include send(16) and receive(16)) */
	unsigned char *buf;      /* IO data buffer base address */
	word_t   dev_base; /* IO device base address */
	volatile sword_t   lock;     /* IO device share lock */
	sword_t            lockcount;/* IO device share lock count */
	volatile sword_t   wait;     /* IO device busy wait */

	sword_t    (*bind)(FILE *);								  /* bind device special configuration to IO device*/
	sword_t    (*open)(FILE *);    							  /* init add a IO device */
	sword_t    (*close)(FILE *);  							  /* deinit and remove a IO device */
	size_t (*read)(FILE *, unsigned char *, size_t);       /* get a device data and status */
	size_t (*write)(FILE *, const unsigned char *, size_t); /* set a device data and status */
	sword_t    (*ioctl)(FILE *, const word_t, sword_t);
	
	struct IO_DEVICE_ITEM *item0; /* instance */
	struct IO_DEVICE_ITEM *item1; /* instance */
};

#define IO_FILE_HEAD(f) (f->item0->u0.head)
#define IO_FILE_PREV(f) (f->item0->u0.prev)
#define IO_FILE_TAIL(f) (f->item0->u1.tail)
#define IO_FILE_NEXT(f) (f->item0->u1.next)

/* device type */
/***************
typedef enum {
    ...,
} FILE_TYPE; 

FILE FILE_TABLE[];
***************/



#endif
