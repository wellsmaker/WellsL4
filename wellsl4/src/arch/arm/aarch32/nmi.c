/**
 * @file
 * @brief NMI handler infrastructure
 *
 * Provides a boot time handler that simply hangs in a sleep loop, and a run
 * time handler that resets the CPU. Also provides a mechanism for hooking a
 * custom run time handler.
 */

#include <arch/cpu.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sys/assert.h>

extern void sys_nmi_reset(void);
#if !defined(CONFIG_RUNTIME_NMI)
#define nmi_handler sys_nmi_reset
#endif

#ifdef CONFIG_RUNTIME_NMI
typedef void (*nmi_handler_t)(void);
static nmi_handler_t nmi_handler = sys_nmi_reset;

/**
 *
 * @brief Default NMI handler installed when kernel is up
 *
 * The default handler outputs a error message and reboots the target. It is
 * installed by calling arm_nmi_init();
 *
 * @return N/A
 */

static void nmi_default_handler(void)
{
	user_error("NMI received! %d Rebooting...\n");
	/* In ARM implementation sys_reboot ignores the parameter */
	sys_arch_reboot(0);
}

/**
 *
 * @brief Install default runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a simple NMI
 * handler that reboots the target. It should be installed after the console is
 * initialized.
 *
 * @return N/A
 */

void arm_nmi_init(void)
{
	nmi_handler = nmi_default_handler;
}

/**
 *
 * @brief Install a custom runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a custom NMI
 * handler that reboots. It should be installed after the console is
 * initialized if it is meant to output to the console.
 *
 * @return N/A
 */

void NmiHandlerSet(void (*pHandler)(void))
{
	nmi_handler = pHandler;
}
#endif /* CONFIG_RUNTIME_NMI */

/**
 *
 * @brief Handler installed in the vector table
 *
 * Simply call what is installed in 'static void(*nmi_handler)(void)'.
 *
 * @return N/A
 */

void arm_nmi(void)
{
	nmi_handler();
	arm_exc_exit();
}
