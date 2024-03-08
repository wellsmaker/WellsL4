# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
  vector_table.S
  reset.S
  fault.c
  scb.c
  irq_init.c
  thread_abort.c
  )

wellsl4_linker_sources_ifdef(CONFIG_SW_VECTOR_RELAY
  TEXT_START
  SORT_KEY 0x0relay_vectors
  relay_vector_table.ld
  )

wellsl4_linker_sources_ifdef(CONFIG_SW_VECTOR_RELAY
  RAM_SECTIONS
  vt_pointer_section.ld
  )
