# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
  vector_table.S
  reset.S
  fault.c
  reboot.c
  stacks.c
  )
