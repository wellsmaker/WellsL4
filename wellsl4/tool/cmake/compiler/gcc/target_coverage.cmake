# SPDX-License-Identifier: Apache-2.0

macro(toolchain_cc_coverage)

wellsl4_compile_options(
  -fprofile-arcs
  -ftest-coverage
  -fno-inline
)

if (NOT CONFIG_COVERAGE_GCOV)

  wellsl4_link_libraries(
    -lgcov
  )

endif()

endmacro()
