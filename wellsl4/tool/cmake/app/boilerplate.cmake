# SPDX-License-Identifier: Apache-2.0

# This file must be included into the toplevel CMakeLists.txt file of
# WellL4 applications, e.g. wellsl4/samples/hello_world/CMakeLists.txt
# must start with the line:
#
# include($ENV{WELLSL4_BASE}/tool/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
#
# It exists to reduce boilerplate code that WellL4 expects to be in
# application CMakeLists.txt code.

# CMake version 3.13.1 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.13.1)

# CMP0002: "Logical target names must be globally unique"
cmake_policy(SET CMP0002 NEW)

# Use the old CMake behaviour until the build scripts have been ported
# to the new behaviour.
# CMP0079: "target_link_libraries() allows use with targets in other directories"
# cmake_policy(SET CMP0079 OLD) 
# The OLD behavior for policy CMP0079 will be removed from a future version of CMake


cmake_policy(SET CMP0116 NEW) 

define_property(GLOBAL PROPERTY WELLSL4_LIBS
    BRIEF_DOCS "Global list of all WellL4 CMake libs that should be linked in"
    FULL_DOCS  "Global list of all WellL4 CMake libs that should be linked in.
wellsl4_library() appends libs to this list.")
set_property(GLOBAL PROPERTY WELLSL4_LIBS "")

define_property(GLOBAL PROPERTY WELLSL4_INTERFACE_LIBS
    BRIEF_DOCS "Global list of all WellL4 interface libs that should be linked in."
    FULL_DOCS  "Global list of all WellL4 interface libs that should be linked in.
wellsl4_interface_library_named() appends libs to this list.")
set_property(GLOBAL PROPERTY WELLSL4_INTERFACE_LIBS "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES
  BRIEF_DOCS "Object files that are generated after WellL4 has been linked once."
  FULL_DOCS "\
Object files that are generated after WellL4 has been linked once.\
May include mmu tables, etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after WellL4 has been linked once."
  FULL_DOCS "\
Source files that are generated after WellL4 has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/wellsl4)

set(PROJECT_BINARY_DIR ${__build_dir})

add_custom_target(code_data_relocation_target)

# CMake's 'project' concept has proven to not be very useful for WellL4
# due in part to how WellL4 is organized and in part to it not fitting well
# with cross compilation.
# WellL4 therefore tries to rely as little as possible on project()
# and its associated variables, e.g. PROJECT_SOURCE_DIR.
# It is recommended to always use WELLSL4_BASE instead of PROJECT_SOURCE_DIR
# when trying to reference ENV${WELLSL4_BASE}.

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "$ENV{WELLSL4_BASE}" PROJECT_SOURCE_DIR)


set(WELLSL4_BINARY_DIR ${PROJECT_BINARY_DIR})
set(WELLSL4_BASE ${PROJECT_SOURCE_DIR})
set(ENV{WELLSL4_BASE}   ${WELLSL4_BASE})

set(AUTOCONF_H ${__build_dir}/inc/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})


#
# Import more CMake functions and macros
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${WELLSL4_BASE}/tool/cmake/extensions.cmake)
include(${WELLSL4_BASE}/tool/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#

include(${WELLSL4_BASE}/tool/cmake/python.cmake)
#include(${WELLSL4_BASE}/tool/cmake/git.cmake)  # depends on version.cmake
include(${WELLSL4_BASE}/tool/cmake/ccache.cmake)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "Source directory equals build directory.\
 In-source builds are not supported.\
 Please specify a build directory, e.g. cmake -Bbuild -H.")
endif()

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${WELLSL4_BASE}/tool/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Dummy add to generate files.
wellsl4_linker_sources(SECTIONS)

# The BOARD can be set by 3 sources. Through environment variables,
# through the cmake CLI, and through CMakeLists.txt.
#
# CLI has the highest precedence, then comes environment variables,
# and then finally CMakeLists.txt.
#
# A user can ignore all the precedence rules if he simply always uses
# the same source. E.g. always specifies -DBOARD= on the command line,
# always has an environment variable set, or always has a set(BOARD
# foo) line in his CMakeLists.txt and avoids mixing sources.
#
# The selected BOARD can be accessed through the variable 'BOARD'.

# Read out the cached board value if present
get_property(cached_board_value CACHE BOARD PROPERTY VALUE)

# There are actually 4 sources, the three user input sources, and the
# previously used value (CACHED_BOARD). The previously used value has
# precedence, and if we detect that the user is trying to change the
# value we give him a warning about needing to clean the build
# directory to be able to change boards.

set(board_cli_argument ${cached_board_value}) # Either new or old
if(board_cli_argument STREQUAL CACHED_BOARD)
  # We already have a CACHED_BOARD so there is no new input on the CLI
  unset(board_cli_argument)
endif()

set(board_app_cmake_lists ${BOARD})
if(cached_board_value STREQUAL BOARD)
  # The app build scripts did not set a default, The BOARD we are
  # reading is the cached value from the CLI
  unset(board_app_cmake_lists)
endif()

if(CACHED_BOARD)
  # Warn the user if it looks like he is trying to change the board
  # without cleaning first
  if(board_cli_argument)
    if(NOT (CACHED_BOARD STREQUAL board_cli_argument))
      message(WARNING "The build directory must be cleaned pristinely when changing boards")
      # TODO: Support changing boards without requiring a clean build
    endif()
  endif()

  set(BOARD ${CACHED_BOARD})
elseif(board_cli_argument)
  set(BOARD ${board_cli_argument})

elseif(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})

elseif(board_app_cmake_lists)
  set(BOARD ${board_app_cmake_lists})

else()
  message(FATAL_ERROR "BOARD is not being defined on the CMake command-line in the environment or by the app.")
endif()

assert(BOARD "BOARD not set")
message(STATUS "Selected BOARD ${BOARD}")

# Store the selected board in the cache
set(CACHED_BOARD ${BOARD} CACHE STRING "Selected board")

# 'BOARD_ROOT' is a prioritized list of directories where boards may
# be found. It always includes ${WELLSL4_BASE} at the lowest priority.
list(APPEND BOARD_ROOT ${WELLSL4_BASE})

if(NOT SOC_ROOT)
  set(SOC_DIR ${WELLSL4_BASE}/src/plat)
else()
  set(SOC_DIR ${SOC_ROOT}/src/plat)
endif()

if(NOT ARCH_ROOT)
  set(ARCH_DIR ${WELLSL4_BASE}/src/arch)
else()
  set(ARCH_DIR ${ARCH_ROOT}/src/arch)
endif()

# Use BOARD to search for a '_defconfig' file.
# e.g. wellsl4/boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig.
# When found, use that path to infer the ARCH we are building for.
foreach(root ${BOARD_ROOT})
  # NB: find_path will return immediately if the output variable is
  # already set
  find_path(BOARD_DIR
    NAMES ${BOARD}_defconfig
    PATHS ${root}/src/plat/*/*
    NO_DEFAULT_PATH
    )
  if(BOARD_DIR AND NOT (${root} STREQUAL ${WELLSL4_BASE}))
    set(USING_OUT_OF_TREE_BOARD 1)
  endif()
endforeach()

if(NOT BOARD_DIR)
  message("No board named '${BOARD}' found")
  print_usage()
  unset(CACHED_BOARD CACHE)
  message(FATAL_ERROR "Invalid usage")
endif()

get_filename_component(BOARD_ARCH_DIR ${BOARD_DIR}      DIRECTORY)
get_filename_component(BOARD_FAMILY   ${BOARD_DIR}      NAME)
get_filename_component(ARCH           ${BOARD_ARCH_DIR} NAME)

if(CONF_FILE)
  # CONF_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable CONF_FILE and the default prj.conf
elseif(DEFINED ENV{CONF_FILE})
  set(CONF_FILE $ENV{CONF_FILE})

elseif(COMMAND set_conf_file)
  message(WARNING "'set_conf_file' is deprecated, it will be removed in a future release.")
  set_conf_file()

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/project_${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/project_${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/project.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)

elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/project.conf)
  set(CONF_FILE ${APPLICATION_SOURCE_DIR}/project.conf)
endif()

set(CONF_FILE ${CONF_FILE} CACHE STRING "If desired, you can build the application using\
the configuration settings specified in an alternate .conf file using this parameter. \
These settings will override the settings in the application’s .config file or its default .conf file.\
Multiple files may be listed, e.g. CONF_FILE=\"project1.conf project2.conf\"")

if(WELLSL4_EXTRA_MODULES)
  # WELLSL4_EXTRA_MODULES has either been specified on the cmake CLI or is
  # already in the CMakeCache.txt. This has precedence over the environment
  # variable WELLSL4_EXTRA_MODULES
elseif(DEFINED ENV{WELLSL4_EXTRA_MODULES})
  set(WELLSL4_EXTRA_MODULES $ENV{WELLSL4_EXTRA_MODULES})
endif()

if(DTC_OVERLAY_FILE)
  # DTC_OVERLAY_FILE has either been specified on the cmake CLI or is already
  # in the CMakeCache.txt. This has precedence over the environment
  # variable DTC_OVERLAY_FILE
elseif(DEFINED ENV{DTC_OVERLAY_FILE})
  set(DTC_OVERLAY_FILE $ENV{DTC_OVERLAY_FILE})
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.overlay)
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/${BOARD}.overlay)
elseif(EXISTS          ${APPLICATION_SOURCE_DIR}/application.overlay)
  set(DTC_OVERLAY_FILE ${APPLICATION_SOURCE_DIR}/application.overlay)
endif()

set(DTC_OVERLAY_FILE ${DTC_OVERLAY_FILE} CACHE STRING "If desired, you can \
build the application using the DT configuration settings specified in an \
alternate .overlay file using this parameter. These settings will override the \
settings in the board's .dts file. Multiple files may be listed, e.g. \
DTC_OVERLAY_FILE=\"dts1.overlay dts2.overlay\"")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

include(${WELLSL4_BASE}/tool/cmake/host-tools.cmake)

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
#
# The DT system uses a C preprocessor for it's code generation needs.
# This creates an awkward chicken-and-egg problem, because we don't
# always know exactly which toolchain the user needs until we know
# more about the target, e.g. after DT and Kconfig.
#
# To resolve this we find "some" C toolchain, configure it generically
# with the minimal amount of configuration needed to have it
# preprocess DT sources, and then, after we have finished processing
# both DT and Kconfig we complete the target-specific configuration,
# and possibly change the toolchain.
include(${WELLSL4_BASE}/tool/cmake/wellsl4_module.cmake)
include(${WELLSL4_BASE}/tool/cmake/generic_toolchain.cmake)
include(${WELLSL4_BASE}/tool/cmake/dts.cmake)
include(${WELLSL4_BASE}/tool/cmake/kconfig.cmake)

#[[
set(SOC_NAME   ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()
]]

include(${WELLSL4_BASE}/tool/cmake/target_toolchain.cmake)

project(WellL4-Kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

# 'project' sets PROJECT_BINARY_DIR to ${CMAKE_CURRENT_BINARY_DIR},
# but for legacy reasons we need it to be set to
# ${CMAKE_CURRENT_BINARY_DIR}/wellsl4
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/wellsl4)
set(PROJECT_SOURCE_DIR ${WELLSL4_BASE})

set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME   ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME   ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME   ${KERNEL_NAME}.hex)
set(KERNEL_MAP_NAME   ${KERNEL_NAME}.map)
set(KERNEL_LST_NAME   ${KERNEL_NAME}.lst)
set(KERNEL_S19_NAME   ${KERNEL_NAME}.s19)
set(KERNEL_EXE_NAME   ${KERNEL_NAME}.exe)
set(KERNEL_STAT_NAME  ${KERNEL_NAME}.stat)
set(KERNEL_STRIP_NAME ${KERNEL_NAME}.strip)

# Populate USER_CACHE_DIR with a directory that user applications may
# write cache files to.
if(NOT DEFINED USER_CACHE_DIR)
  find_appropriate_cache_directory(USER_CACHE_DIR)
endif()
message(STATUS "Cache files will be written to: ${USER_CACHE_DIR}")

include(${BOARD_DIR}/board.cmake OPTIONAL)

# If we are using a suitable ethernet driver inside qemu, then these options
# must be set, otherwise a wellsl4 instance cannot receive any network packets.
# The Qemu supported ethernet driver should define CONFIG_ETH_NIC_MODEL
# string that tells what nic model Qemu should use.
if(CONFIG_QEMU_TARGET)
  if(CONFIG_NET_QEMU_ETHERNET)
    if(CONFIG_ETH_NIC_MODEL)
      list(APPEND QEMU_FLAGS_${ARCH}
        -nic tap,model=${CONFIG_ETH_NIC_MODEL},script=no,downscript=no,ifname=zeth
      )
    else()
      message(FATAL_ERROR "
        No Qemu ethernet driver configured!
        Enable Qemu supported ethernet driver like e1000 at drivers/ethernet"
      )
    endif()
  else()
    list(APPEND QEMU_FLAGS_${ARCH}
      -net none
    )
  endif()
endif()

# General purpose WellL4 target.
# This target can be used for custom wellsl4 settings that needs to be used elsewhere in the build system
#
# Currently used properties:
# - COMPILES_OPTIONS: Used by application memory partition feature
add_custom_target(wellsl4_property_target)

# "app" is a CMake library containing all the application code and is
# modified by the entry point ${APPLICATION_SOURCE_DIR}/CMakeLists.txt
# that was specified when cmake was called.
wellsl4_library_named(app)
set_property(TARGET app PROPERTY ARCHIVE_OUTPUT_DIRECTORY app)

add_subdirectory(${WELLSL4_BASE} ${__build_dir})

# Link 'app' with the WellL4 interface libraries.
#
# NB: This must be done in boilerplate.cmake because 'app' can only be
# modified in the CMakeLists.txt file that created it. And it must be
# done after 'add_subdirectory(${WELLSL4_BASE} ${__build_dir})'
# because interface libraries are defined while processing that
# subdirectory.
get_property(WELLSL4_INTERFACE_LIBS_PROPERTY GLOBAL PROPERTY WELLSL4_INTERFACE_LIBS)
foreach(boilerplate_lib ${WELLSL4_INTERFACE_LIBS_PROPERTY})
  # Linking 'app' with 'boilerplate_lib' causes 'app' to inherit the INTERFACE
  # properties of 'boilerplate_lib'. The most common property is 'include
  # directories', but it is also possible to have defines and compiler
  # flags in the interface of a library.
  #
  string(TOUPPER ${boilerplate_lib} boilerplate_lib_upper_case) # Support lowercase lib names
  target_link_libraries_ifdef(
    CONFIG_APP_LINK_WITH_${boilerplate_lib_upper_case}
    app
    PUBLIC
    ${boilerplate_lib}
    )
endforeach()

if("${CMAKE_EXTRA_GENERATOR}" STREQUAL "Eclipse CDT4")
  # Call the amendment function before .project and .cproject generation
  # C and CXX includes, defines in .cproject without __cplusplus
  # with project includes and defines
  include(${WELLSL4_BASE}/tool/cmake/ide/eclipse_cdt4_generator_amendment.cmake)
  eclipse_cdt4_generator_amendment(1)
endif()
