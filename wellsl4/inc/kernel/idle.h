#ifndef KERNEL_IDLE_H_
#define KERNEL_IDLE_H_

#include <arch/thread.h>
#include <kernel_object.h>
#include <types_def.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_set_power_idle(void);
void sys_set_power_and_idle_exit(word_t ticks);
void idle_thread_entry(void *unused1, void *unused2, void *unused3);

#ifdef __cplusplus
}
#endif
#endif