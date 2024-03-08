# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(uart_stm32.c)

include_directories(
        ${BSP_DIR}/inc/drivers
)