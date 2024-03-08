# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
        cnode.c
        interrupt.c
        ipc.c
        kip.c
        objecttype.c
        tcb.c
        untype.c
        reply.c
)

wellsl4_library_sources_ifdef(
        CONFIG_USERSPACE
        anode.c
)

include_directories(
        ${WELLSL4_BASE}/inc/object
)