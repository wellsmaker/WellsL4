# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
        entryhandler.c
        preemption.c
        sporadic.c
)

wellsl4_library_sources_ifdef(
        CONFIG_SMP
        smp.c
)

wellsl4_library_sources_ifdef(
        CONFIG_ATOMIC_OPERATIONS_C
        atomic.c
)

include_directories(
        ${WELLSL4_BASE}/inc/model
)