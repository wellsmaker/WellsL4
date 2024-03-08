# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a wellsl4_library because it
# should not be --whole-archive'd
add_library(kernel
        boot.c
        device.c
        idle.c
        privilege.c
        thread.c
        time.c
  )

# Kernel files has the macro __WELLSL4_SUPERVISOR__ set so that it
# optimizes the code when userspace is enabled.
set_target_properties(
  kernel
  PROPERTIES
  COMPILE_DEFINITIONS
  __WELLSL4_SUPERVISOR__
  )

target_sources_ifdef(CONFIG_STACK_CANARIES        kernel PRIVATE stack.c)

# The last 2 files inside the target_sources_ifdef should be
# userspace_handler.c and userspace.c. If not the linker would complain.
# This order has to be maintained. Any new file should be placed
# above these 2 files.
target_sources(
  kernel PRIVATE
  cspace.c
  )

target_include_directories(kernel PRIVATE
  ${WELLSL4_BASE}/inc/kernel
  ${WELLSL4_BASE}/inc/arch
  )

add_dependencies(kernel ${OFFSETS_H_TARGET})

target_link_libraries(kernel wellsl4_interface)
