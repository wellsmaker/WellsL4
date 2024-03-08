#ifndef PRIVILEGE_H_
#define PRIVILEGE_H_

#include <types_def.h>

enum privilege_type {
	/* user & privilege */
	/* With the help of this mechanism, FastIPC can realize */
	priv_fastipc_priv = 0, 
	/* privilege supervision mechanism */
	priv_health_monitor_priv,
	priv_debugging_monitor_priv,
	/* others */
	priv_end_priv
};

enum privilege_status {
	priv_ready_priv = 0,
	priv_blocked_priv,
	priv_inactive_priv
};


void set_privilege_status(word_t type, s32_t status);
s32_t get_privilege_status(word_t type);

#endif