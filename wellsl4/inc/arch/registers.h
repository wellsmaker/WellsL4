#ifndef ARCH_REGISTERS_H_
#define ARCH_REGISTERS_H_

#include <types_def.h>
#include <api/errno.h>
#include <kernel_object.h>

exception_t do_control_registers(struct ktcb *thread,
	word_t id,
	word_t mask,
	word_t reg);

/*
__syscall exception_t space_control(
	word_t space_id,
	word_t control,
	word_t kip_page,
	word_t utcb_page,
	word_t thread_id);

__syscall exception_t processor_control(
	word_t processor,
	word_t internal_freq,
	word_t external_freq,
	word_t voltage);
*/

#endif
