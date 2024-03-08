# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
        fatal.c
)

wellsl4_library_sources_ifdef(
        CONFIG_USERSPACE
        syscall.c
)
wellsl4_library_sources_ifdef(
        CONFIG_ERRNO
        errno.c
)
include_directories(
        ${WELLSL4_BASE}/inc/api
)