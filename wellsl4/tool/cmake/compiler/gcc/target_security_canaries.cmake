# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro
macro(toolchain_cc_security_canaries)

  wellsl4_compile_options(-fstack-protector-all)

  # Only a valid option with GCC 7.x and above
  wellsl4_cc_option(-mstack-protector-guard=global)

endmacro()
