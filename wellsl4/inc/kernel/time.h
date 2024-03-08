#ifndef KERNEL_TIME_H_
#define KERNEL_TIME_H_

#include <types_def.h>
#include <sys/dlist.h>
#include <object/tcb.h>
#include <sys/assert.h>
#include <sys/stdbool.h>
#include <arch/thread.h>
#include <kernel_object.h>
#include <api/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

void add_to_timelist(struct timer_event *to, timer_handler_t handler, generptr_t data, 
	sword_t ticks);
void remove_from_timelist(struct timer_event *to);
void readd_to_timelist(struct timer_event *to, word_t dvalue);

sword_t get_next_timelist(void);
void set_next_timelist(sword_t ticks, bool_t idle);
sword_t add_up_timelist(struct timer_event *timeout);
void update_timelist(sword_t ticks);

uint64_t get_current_tick(void);
u32_t get_current_tick_32(void);
#ifndef CONFIG_SYS_CLOCK_EXISTS
#define get_current_tick() (0)
#define get_current_tick_32() (0)
#endif
void set_deadline(ticks_t deadline, word_t *deadline_gid);
u64_t get_uptime_64(void);
void init_time_object(void);

/*
__syscall exception_t system_clock(dword_t *clock)
{
	return EXCEPTION_NONE;

}
*/

static FORCE_INLINE void initialize_timelist(struct timer_event *to)
{
	sys_dnode_init(&to->index);
}

static FORCE_INLINE bool is_inactive_timelist(struct timer_event *to)
{
	return !sys_dnode_is_linked(&to->index);
}

static FORCE_INLINE bool is_active_timelist(struct timer_event *to)
{
	return sys_dnode_is_linked(&to->index);
}


static FORCE_INLINE times_t ticks_to_us(ticks_t ticks)
{
	return(ticks);
}

static FORCE_INLINE ticks_t us_to_ticks(times_t times)
{
	return (times);
}

static FORCE_INLINE ticks_t get_max_ticks(void)
{
	return(0xFFFFFFFFFFFFFFFF);
}

/* this time need to change for some tests */
static FORCE_INLINE ticks_t get_timer_precision(void)
{
	return(0);
}

static FORCE_INLINE times_t get_kernel_wcet_us(void)
{
	return(10u);
}

static FORCE_INLINE ticks_t get_kernel_wcet_ticks(void)
{
	return(10u);
}

/* number of nsec per usec */
#define NSEC_PER_USEC 1000U

/* number of microseconds per millisecond */
#define USEC_PER_MSEC 1000U

/* number of milliseconds per second */
#define MSEC_PER_SEC 1000U

/* number of microseconds per second */
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))

/* number of nanoseconds per second */
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))


/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @note
 *    @rst
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    :option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` config option.
 *    @endrst
 *
 * @return Current uptime in milliseconds.
 */
/* u64_t get_uptime_64(void); */

/**
 * @brief Get system uptime (32-bit version).
 *
 * This routine returns the lower 32 bits of the system uptime in
 * milliseconds.
 *
 * Because correct conversion requires full precision of the system
 * clock there is no benefit to using this over get_uptime_64() unless
 * you know the application will never run long enough for the system
 * clock to approach 2^32 ticks.  Calls to this function may involve
 * interrupt blocking and 64-bit math.
 *
 * @note
 *    @rst
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    :option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` config option
 *    @endrst
 *
 * @return The low 32 bits of the current uptime, in milliseconds.
 */
static FORCE_INLINE word_t get_uptime_32(void)
{
	return (word_t)get_uptime_64();
}

/**
 * @brief Get elapsed time.
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
static FORCE_INLINE s64_t get_uptime_delta_64(s64_t *reftime)
{
	s64_t uptime, delta;

	uptime = get_uptime_64();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

/**
 * @brief Get elapsed time (32-bit version).
 *
 * This routine computes the elapsed time between the current system uptime
 * and an earlier reference time, in milliseconds.
 *
 * This routine can be more efficient than get_uptime_delta_64(), as it reduces the
 * need for interrupt locking and 64-bit math. However, the 32-bit result
 * cannot hold an elapsed time larger than approximately 50 days, so the
 * caller must handle possible rollovers.
 *
 * @param reftime Pointer to a reference time, which is updated to the current
 *                uptime upon return.
 *
 * @return Elapsed time.
 */
static FORCE_INLINE word_t get_uptime_delta_32(s64_t *reftime)
{
	return (word_t)get_uptime_delta_64(reftime);
}

/**
 * @brief Read the hardware clock.
 *
 * This routine returns the current time, as measured by the system's hardware
 * clock.
 *
 * @return Current hardware clock up-counter (in cycles).
 */
static FORCE_INLINE word_t get_cycle_32(void)
{
	return arch_k_cycle_get_32();
}

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define NO_WAIT 0

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define MSEC(ms)     (ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define SECONDS(s)   MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.

 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a m minutes to perform the requested operation.
 *
 * @param m Duration in minutes.
 *
 * @return Timeout delay value.
 */
#define MINUTES(m)   SECONDS((m) * 60)

/**
 * @brief Generate timeout delay from hours.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a h hours to perform the requested operation.
 *
 * @param h Duration in hours.
 *
 * @return Timeout delay value.
 */
#define HOURS(h)     MINUTES((h) * 60)

/**
 * @brief Generate infinite timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define FOREVER (-1)

/* Exhaustively enumerated, highly optimized time unit conversion API */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
static FORCE_INLINE sword_t clock_hw_cycles_per_sec_runtime_get(void)
{
	extern sword_t clock_hw_cycles_per_sec;

	return clock_hw_cycles_per_sec;
}
#endif

static FORCE_INLINE sword_t sys_clock_hw_cycles_per_sec(void)
{
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
	return clock_hw_cycles_per_sec_runtime_get();
#else
	return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif
}

/* Time converter generator gadget.  Selects from one of three
 * conversion algorithms: ones that take advantage when the
 * frequencies are an integer ratio (in either direction), or a full
 * precision conversion.  Clever use of extra arguments causes all the
 * selection logic to be optimized out, and the generated code even
 * reduces to 32 bit only if a ratio conversion is available and the
 * result is 32 bits.
 *
 * This isn't intended to be used directly, instead being wrapped
 * appropriately in a user-facing API.  The boolean arguments are:
 *
 *    const_hz  - The hz arguments are known to be compile-time
 *                constants (because otherwise the modulus test would
 *                have to be done at runtime)
 *    result32  - The result will be truncated to 32 bits on use
 *    round_up  - Return the ceiling of the resulting fraction
 *    round_off - Return the nearest value to the resulting fraction
 *                (pass both round_up/off as false to get "round_down")
 */
static FORCE_INLINE u64_t tmcvt(u64_t t, u32_t from_hz, u32_t to_hz,
				   bool const_hz, bool result32,
				   bool round_up, bool round_off)
{
	bool mul_ratio = const_hz &&
		(to_hz > from_hz) && ((to_hz % from_hz) == 0);
	bool div_ratio = const_hz &&
		(from_hz > to_hz) && ((from_hz % to_hz) == 0);

	if (from_hz == to_hz)
	{
		return result32 ? ((u32_t)t) : t;
	}

	u64_t off = 0;

	if (!mul_ratio) 
	{
		u32_t rdivisor = div_ratio ? (from_hz / to_hz) : from_hz;

		if (round_up)
		{
			off = rdivisor - 1;
		} 
		else if (round_off) 
		{
			off = rdivisor / 2;
		}
	}

	/* Select (at build time!) between three different expressions for
	 * the same mathematical relationship, each expressed with and
	 * without truncation to 32 bits (I couldn't find a way to make
	 * the compiler correctly guess at the 32 bit result otherwise).
	 */
	if (div_ratio) 
	{
		t += off;
		if (result32)
		{
			return ((u32_t)t) / (from_hz / to_hz);
		} 
		else
		{
			return t / (from_hz / to_hz);
		}
	} 
	else if (mul_ratio)
	{
		if (result32) 
		{
			return ((u32_t)t) * (to_hz / from_hz);
		} 
		else
		{
			return t * (to_hz / from_hz);
		}
	} 
	else 
	{
		if (result32) 
		{
			return (u32_t)((t * to_hz + off) / from_hz);
		} 
		else
		{
			return (t * to_hz + off) / from_hz;
		}
	}
}

/* The following code is programmatically generated using this perl
 * code, which enumerates all possible combinations of units, rounding
 * modes and precision.  Do not edit directly.
 *
 * Note that nano/microsecond conversions are only defined with 64 bit
 * precision.  These units conversions were not available in 32 bit
 * variants historically, and doing 32 bit math with units that small
 * has precision traps that we probably don't want to support in an
 * official API.
 *
 * #!/usr/bin/perl -w
 * use strict;
 *
 * my %human = ("ms" => "milliseconds",
 *              "us" => "microseconds",
 *              "ns" => "nanoseconds",
 *              "cyc" => "hardware cycles",
 *              "ticks" => "ticks");
 *
 * sub big { return $_[0] eq "us" || $_[0] eq "ns"; }
 * sub prefix { return $_[0] eq "ms" || $_[0] eq "us" || $_[0] eq "ns"; }
 *
 * for my $from_unit ("ms", "us", "ns", "cyc", "ticks") {
 *     for my $to_unit ("ms", "us", "ns", "cyc", "ticks") {
 *         next if $from_unit eq $to_unit;
 *         next if prefix($from_unit) && prefix($to_unit);
 *         for my $round ("floor", "near", "ceil") {
 *             for(my $big=0; $big <= 1; $big++) {
 *                 my $sz = $big ? 64 : 32;
 *                 my $sym = "k_${from_unit}_to_${to_unit}_$round$sz";
 *                 my $type = "u${sz}_t";
 *                 my $const_hz = ($from_unit eq "cyc" || $to_unit eq "cyc")
 *                     ? "CCYC" : "true";
 *                 my $ret32 = $big ? "false" : "true";
 *                 my $rup = $round eq "ceil" ? "true" : "false";
 *                 my $roff = $round eq "near" ? "true" : "false";
 *
 *                 my $hfrom = $human{$from_unit};
 *                 my $hto = $human{$to_unit};
 *                 print "/", "** \@brief Convert $hfrom to $hto\n";
 *                 print " *\n";
 *                 print " * Converts time values in $hfrom to $hto.\n";
 *                 print " * Computes result in $sz bit precision.\n";
 *                 if ($round eq "ceil") {
 *                     print " * Rounds up to the next highest output unit.\n";
 *                 } elsif ($round eq "near") {
 *                     print " * Rounds to the nearest output unit.\n";
 *                 } else {
 *                     print " * Truncates to the next lowest output unit.\n";
 *                 }
 *                 print " *\n";
 *                 print " * \@return The converted time value\n";
 *                 print " *", "/\n";
 *
 *                 print "static FORCE_INLINE $type $sym($type t)\n{\n\t";
 *                 print "/", "* Generated.  Do not edit.  See above. *", "/\n\t";
 *                 print "return tmcvt(t, H$from_unit, H$to_unit,";
 *                 print " $const_hz, $ret32, $rup, $roff);\n";
 *                 print "}\n\n";
 *             }
 *         }
 *     }
 * }
 */

/* Some more concise declarations to simplify the generator script and
 * save bytes below
 */
#define Hms 1000
#define Hus 1000000
#define Hns 1000000000
#define Hcyc sys_clock_hw_cycles_per_sec()
#define Hticks CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CCYC (!IS_ENABLED(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME))

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_cyc_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, true, false, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_cyc_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, false, false, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_cyc_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, true, false, true);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_cyc_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, false, false, true);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_cyc_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, true, true, false);
}

/** @brief Convert milliseconds to hardware cycles
 *
 * Converts time values in milliseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_cyc_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hcyc, CCYC, false, true, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_ticks_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, true, false, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_ticks_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, false, false, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_ticks_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, true, false, true);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_ticks_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, false, false, true);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ms_to_ticks_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, true, true, false);
}

/** @brief Convert milliseconds to ticks
 *
 * Converts time values in milliseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ms_to_ticks_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hms, Hticks, true, false, true, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_cyc_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, true, false, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_cyc_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, false, false, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_cyc_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, true, false, true);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_cyc_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, false, false, true);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_cyc_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, true, true, false);
}

/** @brief Convert microseconds to hardware cycles
 *
 * Converts time values in microseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_cyc_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hcyc, CCYC, false, true, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_ticks_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, true, false, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_ticks_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, false, false, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_ticks_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, true, false, true);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_ticks_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, false, false, true);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_us_to_ticks_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, true, true, false);
}

/** @brief Convert microseconds to ticks
 *
 * Converts time values in microseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_us_to_ticks_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hus, Hticks, true, false, true, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_cyc_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, true, false, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_cyc_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, false, false, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_cyc_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, true, false, true);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_cyc_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, false, false, true);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_cyc_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, true, true, false);
}

/** @brief Convert nanoseconds to hardware cycles
 *
 * Converts time values in nanoseconds to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_cyc_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hcyc, CCYC, false, true, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_ticks_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, true, false, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_ticks_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, false, false, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_ticks_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, true, false, true);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_ticks_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, false, false, true);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ns_to_ticks_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, true, true, false);
}

/** @brief Convert nanoseconds to ticks
 *
 * Converts time values in nanoseconds to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ns_to_ticks_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hns, Hticks, true, false, true, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ms_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, true, false, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ms_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, false, false, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ms_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, true, false, true);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ms_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, false, false, true);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ms_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, true, true, false);
}

/** @brief Convert hardware cycles to milliseconds
 *
 * Converts time values in hardware cycles to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ms_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hms, CCYC, false, true, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_us_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, true, false, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_us_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, false, false, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_us_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, true, false, true);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_us_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, false, false, true);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_us_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, true, true, false);
}

/** @brief Convert hardware cycles to microseconds
 *
 * Converts time values in hardware cycles to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_us_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hus, CCYC, false, true, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ns_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, true, false, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ns_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, false, false, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ns_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, true, false, true);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ns_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, false, false, true);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ns_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, true, true, false);
}

/** @brief Convert hardware cycles to nanoseconds
 *
 * Converts time values in hardware cycles to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ns_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hns, CCYC, false, true, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ticks_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, true, false, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ticks_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, false, false, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ticks_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, true, false, true);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ticks_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, false, false, true);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_cyc_to_ticks_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, true, true, false);
}

/** @brief Convert hardware cycles to ticks
 *
 * Converts time values in hardware cycles to ticks.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_cyc_to_ticks_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hcyc, Hticks, CCYC, false, true, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ms_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, true, false, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ms_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, false, false, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ms_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, true, false, true);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ms_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, false, false, true);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ms_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, true, true, false);
}

/** @brief Convert ticks to milliseconds
 *
 * Converts time values in ticks to milliseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ms_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hms, true, false, true, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_us_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, true, false, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_us_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, false, false, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_us_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, true, false, true);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_us_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, false, false, true);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_us_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, true, true, false);
}

/** @brief Convert ticks to microseconds
 *
 * Converts time values in ticks to microseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_us_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hus, true, false, true, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ns_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, true, false, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ns_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, false, false, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ns_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, true, false, true);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ns_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, false, false, true);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_ns_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, true, true, false);
}

/** @brief Convert ticks to nanoseconds
 *
 * Converts time values in ticks to nanoseconds.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_ns_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hns, true, false, true, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_cyc_floor32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, true, false, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Truncates to the next lowest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_cyc_floor64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, false, false, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_cyc_near32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, true, false, true);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds to the nearest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_cyc_near64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, false, false, true);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 32 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u32_t k_ticks_to_cyc_ceil32(u32_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, true, true, false);
}

/** @brief Convert ticks to hardware cycles
 *
 * Converts time values in ticks to hardware cycles.
 * Computes result in 64 bit precision.
 * Rounds up to the next highest output unit.
 *
 * @return The converted time value
 */
static FORCE_INLINE u64_t k_ticks_to_cyc_ceil64(u64_t t)
{
	/* Generated.  Do not edit.  See above. */
	return tmcvt(t, Hticks, Hcyc, CCYC, false, true, false);
}

#ifdef CONFIG_TICKLESS_KERNEL
extern sword_t _sys_clock_always_on;
extern void enable_sys_clock(void);
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0)
#error "SYS_CLOCHW_CYCLES_PER_SEC must be non-zero!"
#endif

/* kernel clocks */

/*
 * We default to using 64-bit intermediates in timescale conversions,
 * but if the HW timer cycles/sec, ticks/sec and ms/sec are all known
 * to be nicely related, then we can cheat with 32 bits instead.
 */

#ifdef CONFIG_SYS_CLOCK_EXISTS
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	(MSEC_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) || \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define _NEED_PRECISE_TICMS_CONVERSION
#endif
#endif

#define __ticks_to_ms(t) __DEPRECATED_MACRO \
	k_ticks_to_ms_floor64((u64_t)(t))
#define __ms_to_ticks(t) \
	((sword_t)k_ms_to_ticks_ceil32((u32_t)(t)))
#define __ticks_to_us(t) __DEPRECATED_MACRO \
	((sword_t)k_ticks_to_us_floor32((u32_t)(t)))
#define __us_to_ticks(t) __DEPRECATED_MACRO \
	((sword_t)k_us_to_ticks_ceil32((u32_t)(t)))
#define sys_clock_hw_cycles_per_tick() __DEPRECATED_MACRO \
	((sword_t)k_ticks_to_cyc_floor32(1U))
#define SYS_CLOCHW_CYCLES_TO_NS64(t) __DEPRECATED_MACRO \
	k_cyc_to_ns_floor64((u64_t)(t))
#define SYS_CLOCHW_CYCLES_TO_NS(t) __DEPRECATED_MACRO \
	((u32_t)k_cyc_to_ns_floor64(t))

/* added tick needed to account for tick in progress */
#define _TICALIGN 1

/*
 * SYS_CLOCHW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCHW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(u32_t)(k_cyc_to_ns_floor64(X) / NCYCLES)

/**
 * @defgroup clock_apis Kernel Clock APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @} end defgroup clock_apis
 */

/**
 *
 * @brief Return the lower part of the _current_thread system tick count
 *
 * @return the _current_thread system tick count
 *
 */
/* u32_t get_current_tick_32(void); */

/**
 *
 * @brief Return the _current_thread system tick count
 *
 * @return the _current_thread system tick count
 *
 */
/* s64_t get_current_tick(void); */

#endif
#ifdef __cplusplus
}
#endif
#endif
