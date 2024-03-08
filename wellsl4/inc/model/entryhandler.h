#ifndef MODEL_ENTRYHANLER_H_
#define MODEL_ENTRYHANLER_H_

#include <toolchain.h>
#include <object/tcb.h>


FUNC_NORETURN void thread_entry_point(ktcb_entry_t entry,
	void *p1, void *p2, void *p3);

#endif