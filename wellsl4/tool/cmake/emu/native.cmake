# SPDX-License-Identifier: Apache-2.0

add_custom_target(run
  COMMAND
  ${APPLICATION_BINARY_DIR}/wellsl4/${KERNEL_EXE_NAME}
  DEPENDS ${logical_target_for_wellsl4_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
