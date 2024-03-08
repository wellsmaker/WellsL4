#ifndef MODEL_SMP_H_
#define MODEL_SMP_H_

#include <types_def.h>
#include <kernel_object.h>

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either swap_thread() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void smp_retrieve_global_lock(struct ktcb *thread);
void smp_release_global_lock(struct ktcb *thread);
void smp_init(void);
bool_t smp_cpu_is_vaild(void);


#endif
