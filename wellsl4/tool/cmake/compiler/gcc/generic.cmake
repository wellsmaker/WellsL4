# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC}   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "WellL4 was unable to find the toolchain. Is the environment misconfigured?
User-configuration:
WELLSL4_TOOLCHAIN_VARIANT: ${WELLSL4_TOOLCHAIN_VARIANT}
Internal variables:
CROSS_COMPILE: ${CROSS_COMPILE}
TOOLCHAIN_HOME: ${TOOLCHAIN_HOME}
")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_QUIET
  ERROR_QUIET
  )
if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
'${CMAKE_C_COMPILER} --version'
"
    )
endif()
