#ifndef TYPES_DEF_H_
#define TYPES_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <sys/stdint.h>
#include <sys/stdbool.h>
#include <toolchain.h>


//typedef unsigned char uint8_t;
//typedef unsigned short uint16_t;
//typedef unsigned int  uint32_t;
//typedef unsigned long uint64_t;
//typedef unsigned long long uint128_t;
//typedef char int8_t;
//typedef short int16_t;
//typedef int int32_t;
//typedef long int64_t;
//typedef long long int128_t;

typedef signed char         s8_t;
typedef signed short        s16_t;
typedef signed int          s32_t;
typedef signed long long    s64_t;

typedef unsigned char      u8_t;
typedef unsigned short      u16_t;
typedef unsigned int        u32_t;
typedef unsigned long long  u64_t;

typedef unsigned char byte_t;
/* 32 bits on ILP32 builds, 64 bits on LP64 builts */
typedef unsigned long       ulong_t;

typedef unsigned short hword_t;
typedef unsigned int word_t;
typedef unsigned long long dword_t;
typedef int sword_t;

typedef char *string;
typedef void *generptr_t;
typedef word_t *bitmapptr_t;
typedef word_t kobjptr_t;
typedef int32_t key_t;
typedef word_t bool_t;
typedef int32_t stat_t;
typedef word_t pptr_t;
typedef word_t handler_t;
typedef unsigned long ssize_t;
typedef unsigned int size_t; 
typedef uint64_t ticks_t;
typedef uint64_t times_t;
typedef word_t  prio_t;
typedef word_t  dom_t;

typedef unsigned int paddr_t;
typedef unsigned int vaddr_t;

typedef u32_t ioport_t;
typedef u32_t reg_t;
typedef uintptr_t maddr_t;


enum base_status {
	FAIL = -1,
	SUCES = 0
};

typedef enum base_status bstat_t;

#define UNUSED_OBJ(obj) (void(obj))
#define TRUE  (bool_t)1
#define FALSE (bool_t)0
#define NONE    0x00000000

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#define offsetof(t, d) __builtin_offsetof(t, d)

#define MASK(n) (BIT(n)-1ul)

#define IS_ALIGNED(n, b) (!((n) & MASK(b)))
/* round "x" up/down to next multiple of "align" (which must be a power of 2) */
/*#define ROUND_DOWN(n, b) (((n) >> (b)) << (b))
#define ROUND_UP(n, b) (((((n) - 1ul) >> (b)) + 1ul) << (b)) */

#include <sys/util.h>

#define PASTE(a, b) a ## b

#define SET_UNIT(s,u) ((s) |= (u))
#define CLEAR_UNIT(s,u) ((s) &= (~u))
#define GET_UNIT(s,u) ((s) & (u))
#define MODIFY_UNIT(u,c,s) (u = (u & ~c) | s)

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

#endif

#ifndef _ASMLANGUAGE

#define NULL ((void *)0)
#define SIZE(bits) (1UL << (bits))
#define UL_CONST(x) PASTE(x, ul)

#define __PACKED       __attribute__((packed))
#define __NAKED 		__attribute__ ((naked))
#define __WEAK         __attribute__((weak))
#define __NORETURN     __attribute__((__noreturn__))
#define __CONST        __attribute__((__const__))
#define __PURE         __attribute__((__pure__))
#define __ALIGN(n)     __attribute__((__aligned__(n)))
#define __FASTCALL     __attribute__((fastcall))
#ifdef __clang__
#define __VISIBLE      /* nothing */
#else
#define __VISIBLE      __attribute__((externally_visible))
#endif
#define __NO_INLINE    __attribute__((noinline))
#define __FORCE_INLINE __attribute__((always_inline))
#define __SECTION(sec) __attribute__((__section__(sec)))
#define __UNUSED       __attribute__((unused))
#define __USED         __attribute__((used))
#define __FASTCALL     __attribute__((fastcall))
#ifdef __clang__
#define __FORCE_O2     /* nothing */
#else
#define __FORCE_O2     __attribute__((optimize("O2")))
#endif
/** MODIFIES: */
void __builtin_unreachable(void);
#define UNREACHABLE()  __builtin_unreachable()
#define __MAY_ALIAS    __attribute__((may_alias))

#define __OFFSETOF(type, member) \
    __builtin_offsetof(type, member)

#ifdef __GNUC__
/* Borrowed from linux/include/linux/compiler.h */
#define __LIKE(x)   __builtin_expect(!!(x), 1)
#define __UNLIKE(x) __builtin_expect(!!(x), 0)
#else
#define __LIKE(x)   (!!(x))
#define __UNLIKE(x) (!!(x))
#endif

/* need that for compiling with c99 instead of gnu99 */
#define asm __asm__

/* Evaluate a Kconfig-provided configuration setting at compile-time. */
#define config_set(macro) _is_set_(macro)
#define _macrotest_1 ,
#define _is_set_(value) _is_set__(_macrotest_##value)
#define _is_set__(comma) _is_set___(comma 1, 0)
#define _is_set___(_, v, ...) v

/* Check the existence of a configuration setting, returning one value if it
 * exists and a different one if it does not */
#define config_ternary(macro, true, false) _config_ternary(macro, true, false)
#define _config_ternary(value, true, false) _config_ternary_(_macrotest_##value, true, false)
#define _config_ternary_(comma, true, false) _config_ternary__(comma true, false)
#define _config_ternary__(_, v, ...) v

sword_t __builtin_clzl(unsigned long x);
sword_t __builtin_ctzl(unsigned long x);

/** MODIFIES: */
/** DONT_TRANSLATE */
/** FNSPEC clzl_spec:
  "\<forall>s. \<Gamma> \<turnstile>
    {\<sigma>. s = \<sigma> \<and> x_' s \<noteq> 0 }
      \<acute>ret__long :== PROC clzl(\<acute>x)
    \<lbrace> \<acute>ret__long = of_nat (word_clz (x_' s)) \<rbrace>"
*/
static FORCE_INLINE long
__CONST clzl(unsigned long x)
{
    return __builtin_clzl(x);
}

/** MODIFIES: */
/** DONT_TRANSLATE */
/** FNSPEC ctzl_spec:
  "\<forall>s. \<Gamma> \<turnstile>
    {\<sigma>. s = \<sigma> \<and> x_' s \<noteq> 0 }
      \<acute>ret__long :== PROC ctzl(\<acute>x)
    \<lbrace> \<acute>ret__long = of_nat (word_ctz (x_' s)) \<rbrace>"
*/
static FORCE_INLINE long
__CONST ctzl(unsigned long x)
{
    return __builtin_ctzl(x);
}

#define CTZL(x) __builtin_ctzl(x)

sword_t __builtin_popcountl(unsigned long x);

/** DONT_TRANSLATE */
/** FNSPEC clzll_spec:
  "\<forall>s. \<Gamma> \<turnstile>
    {\<sigma>. s = \<sigma> \<and> x_' s \<noteq> 0 }
      \<acute>ret__longlong :== PROC clzll(\<acute>x)
    \<lbrace> \<acute>ret__longlong = of_nat (word_clz (x_' s)) \<rbrace>"
*/
static FORCE_INLINE long long __CONST clzll(unsigned long long x)
{
    return __builtin_clzll(x);
}

/** DONT_TRANSLATE */
static FORCE_INLINE long
__CONST popcountl(unsigned long mask)
{
#ifndef __POPCNT__
    word_t count; // c accumulates the total bits set in v
    for (count = 0; mask; count++) {
        mask &= mask - 1; // clear the least significant bit set
    }

    return count;
#else
    return __builtin_popcountl(mask);
#endif
}

#define POPCOUNTL(x) popcountl(x)

/* Can be used to insert padding to the next L1 cache line boundary */
#define PAD_TO_NEXT_CACHE_LN(used) char padding[L1_CACHE_LINE_SIZE - ((used) % L1_CACHE_LINE_SIZE)]

#else /* __ASSEMBLER__ */

/* Some assemblers don't recognise ul (unsigned long) suffix */
#define BIT(n) (1 << (n))
#define UL_CONST(x) x

#endif /* !__ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif
