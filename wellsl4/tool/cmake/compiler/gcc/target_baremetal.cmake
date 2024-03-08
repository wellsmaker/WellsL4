# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_nostdinc)

  if (NOT CONFIG_NEWLIB_LIBC AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_NATIVE_APPLICATION)
    wellsl4_compile_options( -nostdinc)
    wellsl4_system_include_directories(${NOSTDINC})
  endif()

endmacro()

macro(toolchain_cc_freestanding)

  wellsl4_compile_options(-ffreestanding)

endmacro()
