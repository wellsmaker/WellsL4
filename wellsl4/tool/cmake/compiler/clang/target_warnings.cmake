# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro

macro(toolchain_cc_warning_base)

  wellsl4_compile_options(
    -Wall
    -Wformat
    -Wformat-security
    -Wno-format-zero-length
    -Wno-main
  )

  wellsl4_cc_option(-Wno-pointer-sign)

  # Prohibit void pointer arithmetic. Illegal in C99
  wellsl4_cc_option(-Wpointer-arith)

endmacro()

macro(toolchain_cc_warning_dw_1)

  wellsl4_compile_options(
    -Wextra
    -Wunused
    -Wno-unused-parameter
    -Wmissing-declarations
    -Wmissing-format-attribute
    )
  wellsl4_cc_option(
    -Wold-style-definition
    -Wmissing-prototypes
    -Wmissing-include-dirs
    -Wunused-but-set-variable
    -Wno-missing-field-initializers
    )

endmacro()

macro(toolchain_cc_warning_dw_2)

  wellsl4_compile_options(
    -Waggregate-return
    -Wcast-align
    -Wdisabled-optimization
    -Wnested-externs
    -Wshadow
    )
  wellsl4_cc_option(
    -Wlogical-op
    -Wmissing-field-initializers
    )

endmacro()

macro(toolchain_cc_warning_dw_3)

  wellsl4_compile_options(
    -Wbad-function-cast
    -Wcast-qual
    -Wconversion
    -Wpacked
    -Wpadded
    -Wpointer-arith
    -Wredundant-decls
    -Wswitch-default
    )
  wellsl4_cc_option(
    -Wpacked-bitfield-compat
    -Wvla
    )

endmacro()

macro(toolchain_cc_warning_extended)

  wellsl4_cc_option(
    #FIXME: need to fix all of those
    -Wno-sometimes-uninitialized
    -Wno-shift-overflow
    -Wno-missing-braces
    -Wno-self-assign
    -Wno-address-of-packed-member
    -Wno-unused-function
    -Wno-initializer-overrides
    -Wno-section
    -Wno-unknown-warning-option
    -Wno-unused-variable
    -Wno-format-invalid-specifier
    -Wno-gnu
    # comparison of unsigned expression < 0 is always false
    -Wno-tautological-compare
    )

endmacro()

macro(toolchain_cc_warning_error_implicit_int)

  # Force an error when things like SYS_INIT(foo, ...) occur with a missing header
  wellsl4_cc_option(-Werror=implicit-int)

endmacro()

#
# The following macros leaves it up to the root CMakeLists.txt to choose
#  the variables in which to put the requested flags, and whether or not
#  to call the macros
#

macro(toolchain_cc_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()

macro(toolchain_cc_cpp_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()

# List the warnings that are not supported for C++ compilations

list(APPEND CXX_EXCLUDED_OPTIONS
  -Werror=implicit-int
  -Wold-style-definition
  )
