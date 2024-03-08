# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

if (CONFIG_COVERAGE)
  toolchain_cc_coverage()
endif ()

wellsl4_library_sources(
  exc_exit.S
  swap.c
  swap_helper.S
  irq_manage.c
  thread.c
  cpu_idle.S
  fault_s.S
  fatal.c
  nmi.c
  nmi_on_reset.S
  prep_c.c
)

wellsl4_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
wellsl4_library_sources_ifdef(CONFIG_CPLUSPLUS __aeabi_atexit.c)
wellsl4_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
wellsl4_library_sources_ifdef(CONFIG_CPU_CORTEX_M0 irq_relay.S)
wellsl4_library_sources_ifdef(CONFIG_USERSPACE userspace.S)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_ARM_MPU cortex_m/mpu)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse)
add_subdirectory_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz)
add_subdirectory_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE cortex_m/tz)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_R cortex_r)

wellsl4_linker_sources(TEXT_START SORT_KEY 0x0vectors vector_table.ld)
