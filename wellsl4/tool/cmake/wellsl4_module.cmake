# SPDX-License-Identifier: Apache-2.0

# This cmake file provides functionality to import additional out-of-tree, OoT
# CMakeLists.txt and Kconfig files into WellL4 build system.
# It uses -DWELLSL4_MODULES=<oot-path-to-module>[;<additional-oot-module(s)>]
# given to CMake for a list of folders to search.
# It looks for: <oot-module>/wellsl4/module.yml or
#               <oot-module>/wellsl4/CMakeLists.txt
# to load the oot-module into WellL4 build system.
# If west is available, it uses `west list` to obtain a list of projects to
# search for wellsl4/module.yml

if(WELLSL4_MODULES)
  set(WELLSL4_MODULES_ARG "--modules" ${WELLSL4_MODULES})
endif()

if(WELLSL4_EXTRA_MODULES)
  set(WELLSL4_EXTRA_MODULES_ARG "--extra-modules" ${WELLSL4_EXTRA_MODULES})
endif()

set(KCONFIG_MODULES_FILE ${CMAKE_BINARY_DIR}/Kconfig.modules)

if(WEST)
  set(WEST_ARG "--west-path" ${WEST})
endif()

if(WEST AND WELLSL4_MODULES)
  # WellL4 module uses west, so only call it if west is installed or
  # WELLSL4_MODULES was provided as argument to CMake.
  execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} ${WELLSL4_BASE}/tool/scripts/wellsl4_module.py
    ${WEST_ARG}
    ${WELLSL4_MODULES_ARG}
    ${WELLSL4_EXTRA_MODULES_ARG}
    --kconfig-out ${KCONFIG_MODULES_FILE}
    --cmake-out ${CMAKE_BINARY_DIR}/wellsl4_modules.txt
    ERROR_VARIABLE
    wellsl4_module_error_text
    RESULT_VARIABLE
    wellsl4_module_return
  )

 if(${wellsl4_module_return})
      message(FATAL_ERROR "${wellsl4_module_error_text}")
  endif()

else()

  file(WRITE ${KCONFIG_MODULES_FILE}
    "# No west and no modules\n"
    )

endif()
