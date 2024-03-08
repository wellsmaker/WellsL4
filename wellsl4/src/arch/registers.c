#include <arch/registers.h>
#include <api/syscall.h>

exception_t do_control_registers(struct ktcb *thread,
	word_t id,
	word_t mask,
	word_t reg)
{
	


	return EXCEPTION_NONE;

}

exception_t syscall_space_control(
	word_t space_id,
	word_t control,
	word_t kip_page,
	word_t utcb_page,
	word_t thread_id)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		return EXCEPTION_NONE;

	}


	return EXCEPTION_FAULT;
}

exception_t syscall_processor_control(
	word_t processor,
	word_t internal_freq,
	word_t external_freq,
	word_t voltage)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		return EXCEPTION_NONE;
	}

	return EXCEPTION_FAULT;

}
