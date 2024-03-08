# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_cpp)

  wellsl4_link_libraries(
    -lstdc++
  )

endmacro()
