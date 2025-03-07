# SPDX-License-Identifier: Apache-2.0

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/inc/generated)

# WellL4 code can configure itself based on a KConfig'uration with the
# header file autoconf.h. There exists an analogous file
# generated_dts_board_unfixed.h that allows configuration based on information
# encoded in DTS.
#
# Here we call on dtc, the gcc preprocessor, and
# scripts/dts/gen_defines.py to generate this header file at
# CMake configure-time.
#
# See ~/wellsl4/doc/dts
set(GENERATED_DTS_BOARD_UNFIXED_H ${PROJECT_BINARY_DIR}/inc/generated/generated_dts_board_unfixed.h)
set(GENERATED_DTS_BOARD_CONF      ${PROJECT_BINARY_DIR}/inc/generated/generated_dts_board.conf)
set(DTS_POST_CPP                  ${PROJECT_BINARY_DIR}/${BOARD}.dts.pre.tmp)

set_ifndef(DTS_SOURCE ${BOARD_DIR}/${BOARD}.dts)

if(DEFINED DTS_COMMON_OVERLAYS)
  # TODO: remove this warning in version 1.16
  message(FATAL_ERROR "DTS_COMMON_OVERLAYS is no longer supported. Use DTC_OVERLAY_FILE instead.")
endif()

# 'DTS_ROOT' is a list of directories where a directory tree with DT
# files may be found. It always includes the application directory,
# the board directory, and ${WELLSL4_BASE}.
list(APPEND
  DTS_ROOT
  ${APPLICATION_SOURCE_DIR}
  ${BOARD_DIR}
  ${WELLSL4_BASE}
  )
list(REMOVE_DUPLICATES
  DTS_ROOT
  )

list(REMOVE_DUPLICATES DTS_ROOT)

set(dts_files
  ${DTS_SOURCE}
  )

# TODO: What to do about non-posix platforms where NOT CONFIG_HAS_DTS (xtensa)?
# Drop support for NOT CONFIG_HAS_DTS perhaps?
if(EXISTS ${DTS_SOURCE})
  set(SUPPORTS_DTS 1)
else()
  set(SUPPORTS_DTS 0)
endif()

if(SUPPORTS_DTS)
  if(DTC_OVERLAY_FILE)
    # Convert from space-separated files into file list
    string(REPLACE " " ";" DTC_OVERLAY_FILE_AS_LIST ${DTC_OVERLAY_FILE})
    list(APPEND
      dts_files
      ${DTC_OVERLAY_FILE_AS_LIST}
      )
  endif()

  set(i 0)
  unset(DTC_INCLUDE_FLAG_FOR_DTS)
  foreach(dts_file ${dts_files})
    list(APPEND DTC_INCLUDE_FLAG_FOR_DTS
         -include ${dts_file})

    if(i EQUAL 0)
      message(STATUS "Loading ${dts_file} as base")
    else()
      message(STATUS "Overlaying ${dts_file}")
    endif()

    math(EXPR i "${i}+1")
  endforeach()

  unset(DTS_ROOT_SYSTEM_INCLUDE_DIRS)
  foreach(dts_root ${DTS_ROOT})
    foreach(dts_root_path
        inc
        tool/dts/dt-bindings
        tool/dts/common
        tool/dts/${ARCH}
        tool/dts
        )
      set(full_path ${dts_root}/${dts_root_path})
      if(EXISTS ${full_path})
        list(APPEND
          DTS_ROOT_SYSTEM_INCLUDE_DIRS
          -isystem ${full_path}
          )
      endif()
    endforeach()
  endforeach()

  unset(DTS_ROOT_BINDINGS)
  foreach(dts_root ${DTS_ROOT})
    set(full_path ${dts_root}/tool/dts/bindings)
    if(EXISTS ${full_path})
      list(APPEND
        DTS_ROOT_BINDINGS
        ${full_path}
        )
    endif()
  endforeach()

  # Cache the location of the root bindings so they can be used by
  # scripts which use the build directory.
  set(CACHED_DTS_ROOT_BINDINGS ${DTS_ROOT_BINDINGS} CACHE INTERNAL
    "DT bindings root directories")

  # TODO: Cut down on CMake configuration time by avoiding
  # regeneration of generated_dts_board_unfixed.h on every configure. How
  # challenging is this? What are the dts dependencies? We run the
  # preprocessor, and it seems to be including all kinds of
  # directories with who-knows how many header files.

  # Run the C preprocessor on an empty C source file that has one or
  # more DTS source files -include'd into it to create the
  # intermediary file *.dts.pre.tmp. Also, generate a dependency file
  # so that changes to DT sources are detected.
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    ${DTS_ROOT_SYSTEM_INCLUDE_DIRS}
    ${DTC_INCLUDE_FLAG_FOR_DTS}  # include the DTS source and overlays
    ${NOSYSDEF_CFLAG}
    -D__DTS__
    -P
    -E   # Stop after preprocessing
    -MD  # Generate a dependency file as a side-effect
    -MF ${PROJECT_BINARY_DIR}/${BOARD}.dts.pre.d
    -o  ${PROJECT_BINARY_DIR}/${BOARD}.dts.pre.tmp
    ${WELLSL4_BASE}/config/empty_file.c
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  # Parse the generated dependency file to find the DT sources that
  # were included and then add them to the list of files that trigger
  # a re-run of CMake.
  toolchain_parse_make_rule(
    ${PROJECT_BINARY_DIR}/${BOARD}.dts.pre.d
    include_files # Output parameter
    )

  set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${include_files}
    )

  # Run the DTC on *.dts.pre.tmp to create the intermediary file *.dts_compiled

  set(DTC_WARN_UNIT_ADDR_IF_ENABLED "")
  check_dtc_flag("-Wunique_unit_address_if_enabled" check)
  if (check)
    set(DTC_WARN_UNIT_ADDR_IF_ENABLED "-Wunique_unit_address_if_enabled")
  endif()
  set(DTC_NO_WARN_UNIT_ADDR "")
  check_dtc_flag("-Wno-unique_unit_address" check)
  if (check)
    set(DTC_NO_WARN_UNIT_ADDR "-Wno-unique_unit_address")
  endif()
  execute_process(
    COMMAND ${DTC}
    -O dts
    -o ${BOARD}.dts_compiled
    -b 0
    -E unit_address_vs_reg
    ${DTC_NO_WARN_UNIT_ADDR}
    ${DTC_WARN_UNIT_ADDR_IF_ENABLED}
    ${EXTRA_DTC_FLAGS} # User settable
    ${BOARD}.dts.pre.tmp
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

  #
  # Run gen_defines.py to create a .conf file and a header file
  #

  set(CMD_NEW_EXTRACT ${PYTHON_EXECUTABLE} ${WELLSL4_BASE}/tool/scripts/dts/gen_defines.py
  --dts ${BOARD}.dts.pre.tmp
  --bindings-dirs ${DTS_ROOT_BINDINGS}
  --conf-out ${GENERATED_DTS_BOARD_CONF}
  --header-out ${GENERATED_DTS_BOARD_UNFIXED_H}
  )

  execute_process(
    COMMAND ${CMD_NEW_EXTRACT}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "new extractor failed with return code: ${ret}")
  endif()

else()
  file(WRITE ${GENERATED_DTS_BOARD_UNFIXED_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif(SUPPORTS_DTS)
