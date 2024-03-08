# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32F429ZI" "--speed=4000")

include(${SOC_DIR}/${ARCH}/common/openocd.board.cmake)
include(${SOC_DIR}/${ARCH}/common/jlink.board.cmake)
