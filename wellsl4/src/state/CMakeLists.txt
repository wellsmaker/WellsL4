# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(
        statedata.c
)

include_directories(
        ${WELLSL4_BASE}/inc/state
)