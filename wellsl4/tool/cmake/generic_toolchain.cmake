# SPDX-License-Identifier: Apache-2.0

if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree toolchain
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default toolchain cmake file
    set(TOOLCHAIN_ROOT ${WELLSL4_BASE})
  endif()
endif()

# Don't inherit compiler flags from the environment
foreach(var CFLAGS CXXFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}},
but WellL4 ignores flags from the environment. Use 'cmake -DEXTRA_${var}=$ENV{${var}}' instead.")
    unset(ENV{${var}})
  endif()
endforeach()

# Until we completely deprecate it
if(NOT DEFINED ENV{WELLSL4_TOOLCHAIN_VARIANT})
  if(DEFINED ENV{WELLSL4_GCC_VARIANT})
    message(WARNING "WELLSL4_GCC_VARIANT is deprecated, please use WELLSL4_TOOLCHAIN_VARIANT instead")
    set(WELLSL4_TOOLCHAIN_VARIANT $ENV{WELLSL4_GCC_VARIANT})
  endif()
endif()

if(NOT WELLSL4_TOOLCHAIN_VARIANT)
  if(DEFINED ENV{WELLSL4_TOOLCHAIN_VARIANT})
    set(WELLSL4_TOOLCHAIN_VARIANT $ENV{WELLSL4_TOOLCHAIN_VARIANT})
  elseif(CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE}))
    set(WELLSL4_TOOLCHAIN_VARIANT cross-compile)
  endif()
endif()

# Until we completely deprecate it
if("${WELLSL4_TOOLCHAIN_VARIANT}" STREQUAL "gccarmemb")
  message(WARNING "gccarmemb is deprecated, please use gnuarmemb instead")
  set(WELLSL4_TOOLCHAIN_VARIANT "gnuarmemb")
endif()

# Host-tools don't unconditionally set TOOLCHAIN_HOME anymore,
# but in case WellL4's SDK toolchain is used, set TOOLCHAIN_HOME
if("${WELLSL4_TOOLCHAIN_VARIANT}" STREQUAL "wellsl4")
  set(TOOLCHAIN_HOME ${HOST_TOOLS_HOME})
endif()

set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "WellL4 toolchain root")
assert(TOOLCHAIN_ROOT "WellL4 toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")

set(WELLSL4_TOOLCHAIN_VARIANT ${WELLSL4_TOOLCHAIN_VARIANT} CACHE STRING "WellL4 toolchain variant")
assert(WELLSL4_TOOLCHAIN_VARIANT "WellL4 toolchain variant invalid: please set the WELLSL4_TOOLCHAIN_VARIANT-variable")

# Pick host system's toolchain if we are targeting posix
if(${ARCH} STREQUAL "posix")
  if(NOT "${WELLSL4_TOOLCHAIN_VARIANT}" STREQUAL "llvm")
    set(WELLSL4_TOOLCHAIN_VARIANT "host")
  endif()
endif()

# Configure the toolchain based on what SDK/toolchain is in use.
include(${TOOLCHAIN_ROOT}/tool/cmake/toolchain/${WELLSL4_TOOLCHAIN_VARIANT}/generic.cmake)

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/tool/cmake/compiler/${COMPILER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/tool/cmake/linker/${LINKER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/tool/cmake/bintools/${BINTOOLS}/generic.cmake OPTIONAL)
