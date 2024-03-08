# SPDX-License-Identifier: Apache-2.0

macro(toolchain_cc_asan)

wellsl4_compile_options(-fsanitize=address)
wellsl4_ld_options(-fsanitize=address)

endmacro()

macro(toolchain_cc_ubsan)

wellsl4_compile_options(-fsanitize=undefined)
wellsl4_ld_options(-fsanitize=undefined)

endmacro()
