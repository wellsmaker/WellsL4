# SPDX-License-Identifier: Apache-2.0

# Lots of duplications here.
# FIXME: maintain this only in one place.

# We need to separate actual toolchain from the host-tools required by WellL4
# and currently provided by the WellL4 SDK. Those tools will need to be
# provided for different OSes and sepearately from the toolchain.

set_ifndef(WELLSL4_SDK_INSTALL_DIR "$ENV{WELLSL4_SDK_INSTALL_DIR}")
set(WELLSL4_SDK_INSTALL_DIR ${WELLSL4_SDK_INSTALL_DIR} CACHE PATH "WellL4 SDK install directory")

if(NOT WELLSL4_SDK_INSTALL_DIR)
  # Until https://github.com/wellsl4project-rtos/wellsl4/issues/4912 is
  # resolved we use WELLSL4_SDK_INSTALL_DIR to determine whether the user
  # wants to use the WellL4 SDK or not.
  return()
endif()

set(REQUIRED_SDK_VER 0.10.3)
set(TOOLCHAIN_VENDOR wellsl4)
set(TOOLCHAIN_ARCH x86_64)

set(sdk_version_path ${WELLSL4_SDK_INSTALL_DIR}/sdk_version)
if(NOT (EXISTS ${sdk_version_path}))
  message(FATAL_ERROR
    "The file '${WELLSL4_SDK_INSTALL_DIR}/sdk_version' was not found. \
Is WELLSL4_SDK_INSTALL_DIR=${WELLSL4_SDK_INSTALL_DIR} misconfigured?")
endif()

# Read version as published by the SDK
file(READ ${sdk_version_path} SDK_VERSION_PRE1)
# Remove any pre-release data, for example 0.10.0-beta4 -> 0.10.0
string(REGEX REPLACE "-.*" "" SDK_VERSION_PRE2 ${SDK_VERSION_PRE1})
# Strip any trailing spaces/newlines from the version string
string(STRIP ${SDK_VERSION_PRE2} SDK_VERSION)
string(REGEX MATCH "([0-9]*).([0-9]*)" SDK_MAJOR_MINOR ${SDK_VERSION})

string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" SDK_MAJOR_MINOR_MICRO ${SDK_VERSION})

#at least 0.0.0
if(NOT SDK_MAJOR_MINOR_MICRO)
  message(FATAL_ERROR "sdk version: ${SDK_MAJOR_MINOR_MICRO} improper format.
  Expected format: x.y.z
  Check whether the WellL4 SDK was installed correctly.
")

elseif(${REQUIRED_SDK_VER} VERSION_GREATER ${SDK_VERSION})
  message(FATAL_ERROR "The SDK version you are using is too old, please update your SDK.
You need at least SDK version ${REQUIRED_SDK_VER}.
You have version ${SDK_VERSION} (${WELLSL4_SDK_INSTALL_DIR}).
The new version of the SDK can be downloaded from:
https://github.com/wellsl4project-rtos/sdk-ng/releases/download/v${REQUIRED_SDK_VER}/wellsl4-sdk-${REQUIRED_SDK_VER}-setup.run
")
endif()

include(${WELLSL4_BASE}/tool/cmake/toolchain/wellsl4/${SDK_MAJOR_MINOR}/host-tools.cmake)
