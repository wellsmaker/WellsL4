#ifndef SYS_BITMAP_H_
#define SYS_BITMAP_H_

#include <types_def.h>

struct bitmap_cursor{
	bitmapptr_t 	bitmap_str;
	key_t 			bpkey;
};
typedef struct bitmap_cursor bitmap_cursor_t;

#define BITMAP_CURSOR_INIT(bitmap,key) \
	(bitmap_cursor_t) {.bitmap_str = bitmap,.bpkey = key}

#define BITMAP_CURSOR_KEY(bitmap_cursor_i) bitmap_cursor_i.bpkey
#define BITMAP_CURSOR_NEXT_KEY(bitmap_cursor_i) bitmap_cursor_i.bpkey++


#define ALIGNNUM(num,align) (num / align) + ((num & (align - 1)) != 0)
#define BITMAP_ALIGN 32
#define DECLARE_BITMAP(name,num) \
	static __BITMAP word_t name[ALIGNNUM(num, BITMAP_ALIGN)];

#define BITMAP_INDEX(bits) (bits / BITMAP_ALIGN)
#define BITMAP_OFFSET(bits) (bits % BITMAP_ALIGN)
#define BITMAP_MEMBER(bitmap_cursor_i) \
	bitmap_cursor_i.bitmap_str[BITMAP_INDEX(bitmap_cursor_i.bpkey)]
#define BITMAP_MASK(bits) (1UL << BITMAP_OFFSET(bits))

static FORCE_INLINE void bitmap_set(bitmap_cursor_t bitmap_cursor_i)
{
    BITMAP_MEMBER(bitmap_cursor_i) |=  BITMAP_MASK(bitmap_cursor_i.bpkey);
}

static FORCE_INLINE void bitmap_clear(bitmap_cursor_t bitmap_cursor_i)
{
    BITMAP_MEMBER(bitmap_cursor_i) &=  ~ BITMAP_MASK(bitmap_cursor_i.bpkey);
}

static FORCE_INLINE word_t bitmap_get(bitmap_cursor_t bitmap_cursor_i)
{
    return (BITMAP_MEMBER(bitmap_cursor_i) & BITMAP_MASK(bitmap_cursor_i.bpkey)) >> BITMAP_OFFSET(bitmap_cursor_i.bpkey);
}

static FORCE_INLINE bool_t bitmap_is_maped(bitmap_cursor_t bitmap_cursor_i)
{
	return (BITMAP_MEMBER(bitmap_cursor_i) & BITMAP_MASK(bitmap_cursor_i.bpkey)) == BITMAP_MASK(bitmap_cursor_i.bpkey);
}

#define FOR_EACH_IN_BITMAP(bitmap_cursor_i,bitmap_i,num,key) \
	for(bitmap_cursor_i = BITMAP_CURSOR_INIT(bitmap_i,key); \
		BITMAP_CURSOR_KEY(bitmap_cursor_i) < num; \
		BITMAP_CURSOR_NEXT_KEY(bitmap_cursor_i))
		
struct kobject_prim{
	string 		 	koname; /*kernel object name*/
	bitmapptr_t	bitmap; /*kernel object bitmap*/
	kobjptr_t 		ko;		/*kernel object*/
	word_t			konum;	/*kernel object number*/
	word_t			kosize; /*kernel object size*/
};

typedef struct kobject_prim kobject_t;

enum kobject_status {
	KEY_OUT_OF_BOUND = -3,
	VALUE_FREE = -2,
	VALUE_ALLOCATED = -1,
	STATUS_NUM = 0
};

typedef enum kobject_status kobject_status_t;

#define DECLARE_KOBJECT(type, name, num) \
	DECLARE_BITMAP(ko_##name##_bitmap, num); \
	static type ko_##name##_object[num]; \
	kobject_t name = { \
			.koname = #name, \
			.bitmap = ko_##name##_bitmap, \
			.ko = (kobjptr_t)ko_##name##_object, \
			.konum = num, \
			.kosize = sizeof(type) \
	}

#define FOR_EACH_IN_KOBJECT(el,key,ko) \
	static bitmap_cursor_t bitmap_cursor_i = BITMAP_CURSOR_INIT((ko)->bitmap,key); \
	for(el = (typeof(ko))(ko)->ko,key = 0;key < (ko)->konum;key++,el++) \
		if(bitmap_is_maped(bitmap_cursor_i))



static FORCE_INLINE void kobject_init(kobject_t *ko)
{
	bitmapptr_t ko_str = ko->bitmap;
	word_t       ko_len = ALIGNNUM(ko->konum,BITMAP_ALIGN);
	word_t       ko_index;
	
	for(ko_index = 0;ko_index < ko_len;ko_index++)
	{
		*(ko_str++) = 0;
	}
}

static FORCE_INLINE kobject_status_t kobject_is_allocated(kobject_t *ko, key_t key)
{
	bitmap_cursor_t	bitmap_cursor_i = BITMAP_CURSOR_INIT(ko->bitmap,key);
	
	if(key > ko->konum)
		return KEY_OUT_OF_BOUND;
	
	if(bitmap_is_maped(bitmap_cursor_i))
		return VALUE_ALLOCATED;
	else
		return VALUE_FREE;
}

static FORCE_INLINE generptr_t kobject_alloc_by_key(kobject_t *ko, key_t key)
{
	bitmap_cursor_t	bitmap_cursor_i = BITMAP_CURSOR_INIT(ko->bitmap,key);

	if(key > ko->konum)
	{
		return NULL;
	}

	if(!bitmap_is_maped(bitmap_cursor_i))
	{
		bitmap_set(bitmap_cursor_i);
		return (generptr_t)(ko->ko + key * ko->kosize);
	}
	else
	{
		return NULL;
	}
}

static FORCE_INLINE generptr_t kobject_alloc(kobject_t *ko)
{
	bitmap_cursor_t	bitmap_cursor_i;

	FOR_EACH_IN_BITMAP(bitmap_cursor_i,ko->bitmap,ko->konum,0)
	{
		if(!bitmap_is_maped(bitmap_cursor_i))
		{
			key_t key = BITMAP_CURSOR_KEY(bitmap_cursor_i);
			bitmap_set(bitmap_cursor_i);
			return (generptr_t)(ko->ko + key * ko->kosize);
		}
	}
	return NULL;
}

static FORCE_INLINE key_t kobject_get_key(kobject_t *ko, generptr_t el)
{
	key_t key = ((kobjptr_t)el - ko->ko) / ko->kosize;
	if(key > ko->konum || key < 0)
		return (key_t)KEY_OUT_OF_BOUND;

	return key;
}

static FORCE_INLINE void kobject_free(kobject_t *ko, generptr_t el)
{
	key_t key = kobject_get_key(ko,el);

	if(key != (key_t)KEY_OUT_OF_BOUND)
	{
		bitmap_cursor_t	bitmap_cursor_i = BITMAP_CURSOR_INIT(ko->bitmap,key);
		bitmap_clear(bitmap_cursor_i);
	}
}

#endif
