# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{WELLSL4_ROOT}/tool/cmake/bsp.cmake NO_POLICY_SCOPE)
include($ENV{WELLSL4_BASE}/tool/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(test1)

target_sources(app PRIVATE src/main.c)
include_directories(
        ${BSP_DIR}/inc/drivers
        
)