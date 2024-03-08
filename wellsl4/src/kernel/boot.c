#include <toolchain.h>
#include <device.h>
#include <sys/string.h>
#include <sys/stdbool.h>
#include <types_def.h>
#include <arch/thread.h>
#include <kernel/boot.h>
#include <user/anode.h>
#include <sys/printk.h>
#include <sys/dlist.h>
#include <model/atomic.h>
#include <linker/sections.h>
#include <linker/linker_defs.h>
#include <kernel/time.h>
#include <kernel/stack.h>
#include <user/anode.h>
#include <arch/cpu.h>
#include <drivers/serial/system_serial.h>
#include <version.h>
/*
 * Symbols used to ensure a rapid series of calls to random number generator
 * return different values.
 */
static atomic_val_t random32_count;
static const word_t random32_dvalue = 1000000013;

/* */
THREAD_STACK_DEFINE(_main_stack, CONFIG_MAIN_STACK_SIZE);
THREAD_STACK_DEFINE(_idle_stack, CONFIG_IDLE_STACK_SIZE);
THREAD_STACK_DEFINE(_privilege_stack, CONFIG_IDLE_STACK_SIZE);

/* idle thread */
struct ktcb privilege_thread;
struct ktcb main_thread;
struct ktcb idle_thread;

/*
 * storage space for the interrupt stack
 *
 * Note: This area is used as the system stack during kernel initialization,
 * since the kernel hasn't yet set up its own stack areas. The dual purposing
 * of this area is safe since interrupts are disabled until the kernel context
 * switches to the init thread.
 */
THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/*
 * Similar idle thread & interrupt stack definitions for the
 * auxiliary CPUs.  The declaration macros aren't set up to define an
 * array, so do it with a simple test for up to 4 processors.  Should
 * clean this up in the future.
 */
#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
THREAD_STACK_DEFINE(_idle_stack1, CONFIG_IDLE_STACK_SIZE);
static struct ktcb idle_thread1_static;
struct ktcb * const idle_thread1 = (struct ktcb *)&idle_thread1_static;
THREAD_STACK_DEFINE(_interrupt_stack1, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
THREAD_STACK_DEFINE(_idle_stack2, CONFIG_IDLE_STACK_SIZE);
static struct ktcb idle_thread2_static;
struct ktcb * const idle_thread2 = (struct ktcb *)&idle_thread2_static;
THREAD_STACK_DEFINE(_interrupt_stack2, CONFIG_ISR_STACK_SIZE);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
THREAD_STACK_DEFINE(_idle_stack3, CONFIG_IDLE_STACK_SIZE);
static struct ktcb idle_thread3_static;
struct ktcb * const idle_thread3 = (struct ktcb *)&idle_thread3_static;
THREAD_STACK_DEFINE(_interrupt_stack3, CONFIG_ISR_STACK_SIZE);
#endif

#if CONFIG_STACK_POINTER_RANDOM
extern word_t stack_adjust_initialized;
#endif

#if defined(CONFIG_MULTITHREADING) && defined(CONFIG_BOOT_DELAY)
#define BOOT_DELAY_BANNER " (delayed boot "	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
#else
#define BOOT_DELAY_BANNER ""
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS) 
extern void init_time_object(void);
#else
#define init_time_object() 
#endif

extern void init_serial_object(void);

extern void idle_thread_entry(void *unused1, void *unused2, void *unused3);
extern void privilege_thread_entry(void *unused1, void *unused2, void *unused3);

#ifdef CONFIG_USERSPACE
/*
 * Application memory region initialization
 */
void set_app_shmem_bss_zero(void)
{
	struct app_region *region, *end;

	end = (struct app_region *)&__app_shmem_regions_end;
	region = (struct app_region *)&__app_shmem_regions_start;

	for (; region < end; region++) 
	{
		(void)memset(region->bss_start, 0, region->bss_size);
	}
}
#endif
/**
 *
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * @return N/A
 */
void set_bss_zero(void)
{
	(void)memset(__bss_start, 0, __bss_end - __bss_start);
	
#if defined(DT_CCM_BASE_ADDRESS) 
	(void)memset(&__ccm_bss_start, 0,
		     ((u32_t) &__ccm_bss_end - (u32_t) &__ccm_bss_start));
#endif
#if defined(DT_DTCM_BASE_ADDRESS) 
	(void)memset(&__dtcm_bss_start, 0,
		     ((u32_t) &__dtcm_bss_end - (u32_t) &__dtcm_bss_start));
#endif
#if defined(CONFIG_CODE_DATA_RELOCATION) 
	extern void bss_zeroing_relocation(void);
	bss_zeroing_relocation();
#endif
#if defined(CONFIG_COVERAGE_GCOV) 
	(void)memset(&__gcov_bss_start, 0,
		 ((u32_t) &__gcov_bss_end - (u32_t) &__gcov_bss_start));
#endif
}


#if defined(CONFIG_XIP) 
/**
 *
 * @brief Copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * @return N/A
 */
void copy_data(void)
{
	(void)memcpy(&__data_ram_start, &__data_rom_start,
		 __data_ram_end - __data_ram_start);
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT) 
	(void)memcpy(&_ramfunc_ram_start, &_ramfunc_rom_start,
		 (uintptr_t) &_ramfunc_ram_size);
#endif
#if defined(DT_CCM_BASE_ADDRESS) 
	(void)memcpy(&__ccm_data_start, &__ccm_data_rom_start,
		 __ccm_data_end - __ccm_data_start);
#endif
#if defined(DT_DTCM_BASE_ADDRESS) 
	(void)memcpy(&__dtcm_data_start, &__dtcm_data_rom_start,
		 __dtcm_data_end - __dtcm_data_start);
#endif
#if defined(CONFIG_CODE_DATA_RELOCATION) 
	extern void data_copy_xip_relocation(void);
	data_copy_xip_relocation();
#endif
#if defined(CONFIG_USERSPACE) 
#if defined(CONFIG_STACCANARIES) 
	/* stack canary checking is active for all C functions.
	 * __stack_chk_guard is some uninitialized value living in the
	 * app shared memory sections. Preserve it, and don't make any
	 * function calls to perform the memory copy. The true canary
	 * value gets set later in cstart().
	 */
	uintptr_t guard_copy = __stack_chk_guard;
	byte_t *src = (byte_t *)&_app_smem_rom_start;
	byte_t *dst = (byte_t *)&_app_smem_start;
	u32_t count = _app_smem_end - _app_smem_start;

	guard_copy = __stack_chk_guard;
	while (count > 0) 
	{
		*(dst++) = *(src++);
		count--;
	}
	__stack_chk_guard = guard_copy;
#else
	(void)memcpy(&_app_smem_start, &_app_smem_rom_start,
		 _app_smem_end - _app_smem_start);
#endif
#endif
}
#endif


/**
 *
 * @brief Mainline for kernel's background thread
 *
 * This routine completes kernel initialization by invoking the remaining
 * init functions, then invokes application's main() routine.
 *
 * @return N/A
 */
static void main_thread_entry(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

#if defined(CONFIG_BOOT_DELAY)
	static const word_t boot_delay = CONFIG_BOOT_DELAY;
#else
	static const word_t boot_delay = 0;
#endif

	do_device_config(sys_init_level_post_kernel);

#if defined(CONFIG_STACK_POINTER_RANDOM) 
	stack_adjust_initialized = true;
#endif

	if (boot_delay > 0 && IS_ENABLED(CONFIG_MULTITHREADING)) 
	{
		printk("***** delaying boot " STRINGIFY(CONFIG_BOOT_DELAY)
		       "ms (per build configuration) *****\r\n");
		thread_busy_wait(CONFIG_BOOT_DELAY * USEC_PER_MSEC);
	}

#if defined(BUILD_VERSION) 
	printk("*** Booting WellsL4 OS build %s %s ***\r\n",
			STRINGIFY(BUILD_VERSION), BOOT_DELAY_BANNER);
#else
	printk("*** Booting WellsL4 OS version %s %s ***\r\n",
			KERNEL_VERSION_STRING, BOOT_DELAY_BANNER);
#endif
	printk("*** Welcome to use the WellsL4 OS ***\r\n");
	printk("*** If you have any comments in the use process, please contact me at wu_peng@careri.com ***\r\n");
	/* Final init level before app starts */
	do_device_config(sys_init_level_application);

#if defined(CONFIG_CPLUSPLUS) 
	/* Process the .ctors and .init_array sections */
	extern void __do_global_ctors_aux(void);
	extern void __do_init_array_aux(void);
	__do_global_ctors_aux();
	__do_init_array_aux();
#endif

#if defined(CONFIG_SMP) 
	smp_init();
#endif

#if defined(CONFIG_BOOT_TIME_MEASUREMENT) 
	timestamp_main = get_cycle_32();
#endif

	extern void main(void);

	main();

	/* Mark nonessenrial since main() has no more work to do */
	main_thread.base.option &= ~option_essential_option;

#if defined(CONFIG_COVERAGE_DUMP) 
	/* Dump coverage data once the main() has exited. */
#endif
}

void __weak main(void)
{
	arch_nop();
}

/**
 *
 * @brief Initializes kernel data structures
 *
 * This routine initializes various kernel data structures, including
 * the init and idle record_threads and any architecture-specific initialization.
 *
 * Note that all fields of "_kernel" are set to zero on entry, which may
 * be all the initialization many of them require.
 *
 * @return N/A
 */
#if defined(CONFIG_MULTITHREADING) 
static void init_idle_thread(struct ktcb *thread, struct thread_stack *stack)
{
	set_new_thread(thread, stack, CONFIG_IDLE_STACK_SIZE, idle_thread_entry, 
		NULL, NULL, NULL, option_essential_option);

	marktcb_as_started(thread);

#if defined(CONFIG_SMP) 
	thread->base.smp_is_idle = true;
#endif
}

static void init_privilege_thread(struct ktcb *thread, struct thread_stack *stack)
{
	set_new_thread(thread, stack, CONFIG_PRIVILEGE_STACK_SIZE, privilege_thread_entry, 
		NULL, NULL, NULL, option_essential_option);
	marktcb_as_started(thread);
}

static void init_main_thread(struct ktcb *thread, struct thread_stack *stack)
{
	set_new_thread(thread, stack,
			   CONFIG_MAIN_STACK_SIZE, main_thread_entry,
			   NULL, NULL, NULL, option_essential_option);
	set_prior(thread, 31u, 31u);
	set_domain(thread, 0u);
	marktcb_as_started(thread);
	set_ready_thread(thread);
}

static void prepare_mult_thread(struct ktcb *dummy_thread)
{
#if defined(CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN) 
	ARG_UNUSED(dummy_thread);
#else

	/*
	 * Initialize the _current_thread execution thread to permit a level of
	 * debugging output if an exception should happen during kernel
	 * initialization.  However, don't waste effort initializing the
	 * fields of the dummy thread beyond those needed to identify it as a
	 * dummy thread.
	 */
	dummy_thread->base.option = option_essential_option;
	dummy_thread->base.thread_state = state_dummy_state;
	dummy_thread->sched = NULL;
#if defined(CONFIG_THREAD_STACK_INFO) 
	dummy_thread->stack_info.start = 0U;
	dummy_thread->stack_info.size = 0U;
#endif
#if defined(CONFIG_USERSPACE) 
	dummy_thread->userspace_fpage_table.pagetable_item = NULL;
#endif
#endif

	/* sched_init(); */

#if !defined(CONFIG_SMP) 
	/*
	 * prime the cache with the main thread since:
	 *
	 * - the cache can never be NULL
	 * - the main thread will be the one to run first
	 * - no other thread is initialized yet and thus their sched_prior fields
	 *   contain garbage, which would prevent the cache loading algorithm
	 *   to work as intended
	 */
	_kernel.ready_thread = &main_thread;
#endif

	/* set_new_thread(&main_thread, _main_stack,
			   CONFIG_MAIN_STACK_SIZE, main_thread_entry,
			   NULL, NULL, NULL, option_essential_option);
	marktcb_as_started(&main_thread); */
	/* set_ready_thread(&main_thread); */

	init_main_thread(&main_thread, _main_stack);

	/* maybe every core has it */
	init_privilege_thread(&privilege_thread, _privilege_stack);
	
	init_idle_thread(&idle_thread, _idle_stack);
	_kernel.cpus[0].idle_thread = &idle_thread;
	_kernel.cpus[0].core_id = 0;
	_kernel.cpus[0].int_stack_point = THREAD_STACK_BUFFER(_interrupt_stack)
		+ CONFIG_ISR_STACK_SIZE;

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	init_idle_thread(idle_thread1, _idle_stack1);
	_kernel.cpus[1].idle_thread = idle_thread1;
	_kernel.cpus[1].core_id = 1;
	_kernel.cpus[1].int_stack_point = THREAD_STACK_BUFFER(_interrupt_stack1)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	init_idle_thread(idle_thread2, _idle_stack2);
	_kernel.cpus[2].idle_thread = idle_thread2;
	_kernel.cpus[2].core_id = 2;
	_kernel.cpus[2].int_stack_point = THREAD_STACK_BUFFER(_interrupt_stack2)
		+ CONFIG_ISR_STACK_SIZE;
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	init_idle_thread(idle_thread3, _idle_stack3);
	_kernel.cpus[3].idle_thread = idle_thread3;
	_kernel.cpus[3].core_id = 3;
	_kernel.cpus[3].int_stack_point = THREAD_STACK_BUFFER(_interrupt_stack3)
		+ CONFIG_ISR_STACK_SIZE;
#endif
}

static FUNC_NORETURN void switch_to_main_thread(void)
{
#if defined(CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN) 
	arch_switch_to_main_thread(&main_thread, _main_stack,
		THREAD_STACK_SIZEOF(_main_stack), main_thread_entry);
#else
	/*
	 * Context switch to main task (entry function is _main()): the
	 * _current_thread fake thread is not on a wait queue or ready queue, so it
	 * will never be rescheduled in.
	 */
	swap_thread_unlocked();
#endif
	CODE_UNREACHABLE;
}
#endif


/**
 *
 * @brief Get a 32 bit random number
 *
 * The non-random number generator returns values that are based off the
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @return a 32-bit number
 */

word_t get_random32_value(void)
{
	return get_cycle_32() + atomic_add(&random32_count, random32_dvalue);
}

void set_random32(void *dst, size_t outlen)
{
	u32_t len = 0;
	u32_t blocksize = 4;
	u32_t ret;
	u32_t *udst = (u32_t *)dst;

	while (len < outlen) 
	{
		ret = get_random32_value();
		
		if ((outlen-len) < sizeof(ret)) 
		{
			blocksize = len;
			(void)memcpy(udst, &ret, blocksize);
		} 
		else 
		{
			(*udst++) = ret;
		}
		
		len += blocksize;
	}
}

void set_random32_canary_word(byte_t *dst_word, size_t word_size)
{
	word_t base_size = sizeof(word_t);
	while (word_size > 0) 
	{
		word_t random32_bits;
		byte_t *random8_bits;

		random32_bits = get_random32_value();
		random8_bits = (byte_t *)&random32_bits;

		if (word_size < base_size) 
		{
			base_size = word_size;
		}

		for (word_t index = 0; index < base_size; index ++) 
		{
			*dst_word = *random8_bits;
			dst_word++;
			random8_bits++;
		}

		word_size -= base_size;
	}
}

/**
 *
 * @brief Initialize kernel
 *
 * This routine is invoked when the system is ready to run C code. The
 * processor must be running in 32-bit mode, and the BSS must have been
 * cleared/zeroed.
 *
 * @return Does not return
 */
FUNC_NORETURN void cstart(void)
{
#if defined(CONFIG_STACCANARIES) 
	uintptr_t stack_guard;
#endif
	/* gcov hook needed to get the coverage report.*/
	/* gcov_static_init(); */
	/* perform any architecture-specific initialization */
	arch_kernel_init((struct interrupt_stack *)_interrupt_stack); /** cast */

#if defined(CONFIG_MULTITHREADING) 
	struct ktcb dummy_thread = 
	{
		 .base.thread_state.obj_state = state_dummy_state,
#if defined(CONFIG_SCHED_CPU_MASK) 
		 .base.smp_cpu_mask = -1,
#endif
	};

	_current_thread = &dummy_thread;
#endif

#if defined(CONFIG_USERSPACE) 
	set_app_shmem_bss_zero();
#endif

	/* perform basic hardware initialization */
	do_device_config(sys_init_level_pre_kernel_1);
	do_device_config(sys_init_level_pre_kernel_2);

	/* time init must before the set_random32_canary_word */
	init_serial_object();
	init_time_object();

#if defined(CONFIG_STACCANARIES) 
	set_random32_canary_word((byte_t *)&stack_guard, sizeof(stack_guard));
	__stack_chk_guard = stack_guard;
	__stack_chk_guard <<= 8;
#endif

#if defined(CONFIG_MULTITHREADING) 
	prepare_mult_thread(&dummy_thread);
 	switch_to_main_thread();
#else
	main_thread_entry(NULL, NULL, NULL);
	/* 
	 * We've already dumped coverage data at this point.
	 */
	irq_lock();
	while (true);
#endif
	/*
	 * Compiler can't tell that the above routines won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */
	CODE_UNREACHABLE;
}
