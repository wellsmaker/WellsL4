/**
 * @file
 * @brief Kernel fatal error handler for ARM64 Cortex-A
 *
 * This module provides the arm64_fatal_error() routine for ARM64 Cortex-A
 * CPUs
 */

#include <api/fatal.h>
#include <sys/assert.h>

static void print_EC_cause(u64_t esr)
{
	u32_t EC = (u32_t)esr >> 26;

	switch (EC) 
	{
		case 0b000000:
			user_error("Unknown reason");
			break;
		case 0b000001:
			user_error("Trapped WFI or WFE instruction execution");
			break;
		case 0b000011:
			user_error("Trapped MCR or MRC access with (coproc==0b1111) that "
				"is not reported using EC 0b000000");
			break;
		case 0b000100:
			user_error("Trapped MCRR or MRRC access with (coproc==0b1111) "
				"that is not reported using EC 0b000000");
			break;
		case 0b000101:
			user_error("Trapped MCR or MRC access with (coproc==0b1110)");
			break;
		case 0b000110:
			user_error("Trapped LDC or STC access");
			break;
		case 0b000111:
			user_error("Trapped access to SVE, Advanced SIMD, or "
				"floating-point functionality");
			break;
		case 0b001100:
			user_error("Trapped MRRC access with (coproc==0b1110)");
			break;
		case 0b001101:
			user_error("Branch Target Exception");
			break;
		case 0b001110:
			user_error("Illegal Execution state");
			break;
		case 0b010001:
			user_error("SVC instruction execution in AArch32 state");
			break;
		case 0b011000:
			user_error("Trapped MSR, MRS or System instruction execution in "
				"AArch64 state, that is not reported using EC "
				"0b000000, 0b000001 or 0b000111");
			break;
		case 0b011001:
			user_error("Trapped access to SVE functionality");
			break;
		case 0b100000:
			user_error("Instruction Abort from a lower Exception level, that "
				"might be using AArch32 or AArch64");
			break;
		case 0b100001:
			user_error("Instruction Abort taken without a change in Exception "
				"level.");
			break;
		case 0b100010:
			user_error("PC alignment fault exception.");
			break;
		case 0b100100:
			user_error("Data Abort from a lower Exception level, that might "
				"be using AArch32 or AArch64");
			break;
		case 0b100101:
			user_error("Data Abort taken without a change in Exception level");
			break;
		case 0b100110:
			user_error("SP alignment fault exception");
			break;
		case 0b101000:
			user_error("Trapped floating-point exception taken from AArch32 "
				"state");
			break;
		case 0b101100:
			user_error("Trapped floating-point exception taken from AArch64 "
				"state.");
			break;
		case 0b101111:
			user_error("SError interrupt");
			break;
		case 0b110000:
			user_error("Breakpoint exception from a lower Exception level, "
				"that might be using AArch32 or AArch64");
			break;
		case 0b110001:
			user_error("Breakpoint exception taken without a change in "
				"Exception level");
			break;
		case 0b110010:
			user_error("Software Step exception from a lower Exception level, "
				"that might be using AArch32 or AArch64");
			break;
		case 0b110011:
			user_error("Software Step exception taken without a change in "
				"Exception level");
			break;
		case 0b110100:
			user_error("Watchpoint exception from a lower Exception level, "
				"that might be using AArch32 or AArch64");
			break;
		case 0b110101:
			user_error("Watchpoint exception taken without a change in "
				"Exception level.");
			break;
		case 0b111000:
			user_error("BKPT instruction execution in AArch32 state");
			break;
		case 0b111100:
			user_error("BRK instruction execution in AArch64 state.");
			break;
	}
}

static void esf_dump(const arch_esf_t *esf)
{
	user_error("x1:  %-8llx  x0:  %llx",
		esf->basic.regs[18], esf->basic.regs[19]);
	user_error("x2:  %-8llx  x3:  %llx",
		esf->basic.regs[16], esf->basic.regs[17]);
	user_error("x4:  %-8llx  x5:  %llx",
		esf->basic.regs[14], esf->basic.regs[15]);
	user_error("x6:  %-8llx  x7:  %llx",
		esf->basic.regs[12], esf->basic.regs[13]);
	user_error("x8:  %-8llx  x9:  %llx",
		esf->basic.regs[10], esf->basic.regs[11]);
	user_error("x10: %-8llx  x11: %llx",
		esf->basic.regs[8], esf->basic.regs[9]);
	user_error("x12: %-8llx  x13: %llx",
		esf->basic.regs[6], esf->basic.regs[7]);
	user_error("x14: %-8llx  x15: %llx",
		esf->basic.regs[4], esf->basic.regs[5]);
	user_error("x16: %-8llx  x17: %llx",
		esf->basic.regs[2], esf->basic.regs[3]);
	user_error("x18: %-8llx  x30: %llx",
		esf->basic.regs[0], esf->basic.regs[1]);
}

void arm64_fatal_error(word_t reason, const arch_esf_t *esf)
{
	u64_t el, esr, elr, far;

	if (reason != fatal_spurious_irq_fatal) 
	{
		__asm__ volatile("mrs %0, CurrentEL" : "=r" (el));

		switch (GET_EL(el)) {
		case MODE_EL1:
			__asm__ volatile("mrs %0, esr_el1" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el1" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el1" : "=r" (elr));
			break;
		case MODE_EL2:
			__asm__ volatile("mrs %0, esr_el2" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el2" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el2" : "=r" (elr));
			break;
		case MODE_EL3:
			__asm__ volatile("mrs %0, esr_el3" : "=r" (esr));
			__asm__ volatile("mrs %0, far_el3" : "=r" (far));
			__asm__ volatile("mrs %0, elr_el3" : "=r" (elr));
			break;
		default:
			/* Just to keep the compiler happy */
			esr = elr = far = 0;
			break;
		}

		if (GET_EL(el) != MODE_EL0)
		{
			user_error("ESR_ELn: %llx", esr);
			user_error("FAR_ELn: %llx", far);
			user_error("ELR_ELn: %llx", elr);

			print_EC_cause(esr);
		}

	}

	if (esf != NULL)
	{
		esf_dump(esf);
	}
	
	fatal_error(reason, esf);
	CODE_UNREACHABLE;
}
