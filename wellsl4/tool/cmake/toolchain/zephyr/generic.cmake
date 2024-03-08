# SPDX-License-Identifier: Apache-2.0

if (NOT WELLSL4_SDK_INSTALL_DIR)
  message(FATAL_ERROR "WELLSL4_SDK_INSTALL_DIR must be set")
endif()

include(${WELLSL4_BASE}/tool/cmake/toolchain/wellsl4/${SDK_MAJOR_MINOR}/generic.cmake)
