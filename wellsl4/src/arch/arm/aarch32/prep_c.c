/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */
#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <linker/linker_defs.h>
#include <kernel/boot.h>
#if defined(CONFIG_ARMV7_R)
#include <arch/arm/aarch32/cortex_r/stack.h>
#endif

#if defined(__GNUC__)
/*
 * GCC can detect if memcpy is passed a NULL argument, however one of
 * the cases of relocate_vector_table() it is valid to pass NULL, so we
 * suppress the warning for this case.  We need to do this before
 * string.h is included to get the declaration of memcpy.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

#include <sys/string.h>

#ifdef CONFIG_CPU_CORTEX_M_HAS_VTOR

#ifdef CONFIG_XIP
#define VECTOR_ADDRESS ((uintptr_t)_vector_start)
#else
#define VECTOR_ADDRESS CONFIG_SRAM_BASE_ADDRESS
#endif
static FORCE_INLINE void relocate_vector_table(void)
{
	SCB->VTOR = VECTOR_ADDRESS & SCB_VTOR_TBLOFF_Msk;
	__DSB();
	__ISB();
}

#else

#if defined(CONFIG_SW_VECTOR_RELAY)
GENERIC_SECTION(.vt_pointer_section) void *_vector_table_pointer;
#endif

#define VECTOR_ADDRESS 0

void __weak relocate_vector_table(void)
{
#if defined(CONFIG_XIP) && (CONFIG_FLASH_BASE_ADDRESS != 0) || \
    !defined(CONFIG_XIP) && (CONFIG_SRAM_BASE_ADDRESS != 0)
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	(void)memcpy(VECTOR_ADDRESS, _vector_start, vector_size);
#elif defined(CONFIG_SW_VECTOR_RELAY)
	_vector_table_pointer = _vector_start;
#endif
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* CONFIG_CPU_CORTEX_M_HAS_VTOR */

#if defined(CONFIG_CPU_HAS_FPU)
static FORCE_INLINE void arm_floating_point_init(void)
{
	/*
	 * Upon reset, the Co-Processor Access Control Register is, normally,
	 * 0x00000000. However, it might be left un-cleared by firmware running
	 * before WellL4 boot.
	 */
	SCB->CPACR &= (~(CPACR_CP10_Msk | CPACR_CP11_Msk));

#if defined(CONFIG_FLOAT)
	/*
	 * Enable CP10 and CP11 Co-Processors to enable access to floating
	 * point registers.
	 */
#if defined(CONFIG_USERSPACE)
	/* Full access */
	SCB->CPACR |= CPACR_CP10_FULL_ACCESS | CPACR_CP11_FULL_ACCESS;
#else
	/* Privileged access only */
	SCB->CPACR |= CPACR_CP10_PRIV_ACCESS | CPACR_CP11_PRIV_ACCESS;
#endif /* CONFIG_USERSPACE */
	/*
	 * Upon reset, the FPU Context Control Register is 0xC0000000
	 * (both Automatic and Lazy state preservation is enabled).
	 */
#if !defined(CONFIG_FP_SHARING)
	/* Default mode is Unshared FP registers mode. We disable the
	 * automatic stacking of FP registers (automatic setting of
	 * FPCA bit in the CONTROL register), upon exception entries,
	 * as the FP registers are to be used by a single context (and
	 * the use of FP registers in ISRs is not supported). This
	 * configuration improves interrupt latency and decreases the
	 * stack memory requirement for the (single) thread that makes
	 * use of the FP co-processor.
	 */
	FPU->FPCCR &= (~(FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk));
#else
	/*
	 * Enable both automatic and lazy state preservation of the FP context.
	 * The FPCA bit of the CONTROL register will be automatically set, if
	 * the thread uses the floating point registers. Because of lazy state
	 * preservation the volatile FP registers will not be stacked upon
	 * exception entry, however, the required area in the stack page_f will
	 * be reserved for them. This configuration improves interrupt latency.
	 * The registers will eventually be stacked when the thread is swapped
	 * out during context-switch.
	 */
	FPU->FPCCR = FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
#endif /* CONFIG_FP_SHARING */

	/* Make the side-effects of modifying the FPCCR be realized
	 * immediately.
	 */
	__DSB();
	__ISB();

	/* Initialize the Floating Point Status and Control Register. */
	__set_FPSCR(0);

	/*
	 * Note:
	 * The use of the FP register bank is enabled, however the FP context
	 * will be activated (FPCA bit on the CONTROL register) in the presence
	 * of floating point instructions.
	 */

#endif

	/*
	 * Upon reset, the CONTROL.FPCA bit is, normally, cleared. However,
	 * it might be left un-cleared by firmware running before WellL4 boot.
	 * We must clear this bit to prevent errors in exception unstacking.
	 *
	 * Note:
	 * In Sharing FP Registers mode CONTROL.FPCA is cleared before switching
	 * to main, so it may be skipped here (saving few boot cycles).
	 */
#if !defined(CONFIG_FLOAT) || !defined(CONFIG_FP_SHARING)
	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
#endif
}
#endif

extern void arm_int_lib_init(void);
extern FUNC_NORETURN void cstart(void);
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */
void arm_prep_c(void)
{
	relocate_vector_table();
#if defined(CONFIG_CPU_HAS_FPU)
	arm_floating_point_init();
#endif
	set_bss_zero();
	copy_data();
#if defined(CONFIG_ARMV7_R) && defined(CONFIG_INIT_STACKS)
	extern void arm_init_stacks(void);
	arm_init_stacks();
#endif
	arm_int_lib_init();
	cstart();
	CODE_UNREACHABLE;
}
