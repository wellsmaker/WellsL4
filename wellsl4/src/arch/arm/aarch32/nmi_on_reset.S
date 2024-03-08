/**
 * @file
 * @brief Default basic NMI handler before the kernel is up
 *
 * Provide a default handler for NMI before the system is up. The default action
 * is to hard hang, sleeping.
 *
 * This might be preferable than rebooting to help debugging, or because
 * rebooting might trigger the exact same problem over and over.
 */

#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(sys_nmi_reset)

SECTION_FUNC(TEXT, sys_nmi_reset)
    wfi
    b sys_nmi_reset