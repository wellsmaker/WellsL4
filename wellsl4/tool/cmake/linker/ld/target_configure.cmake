# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_configure_files)
  configure_file(
       $ENV{WELLSL4_BASE}/inc/linker/app_data_alignment.ld
       ${PROJECT_BINARY_DIR}/inc/generated/app_data_alignment.ld)

  configure_file(
       $ENV{WELLSL4_BASE}/inc/linker/app_smem.ld
       ${PROJECT_BINARY_DIR}/inc/generated/app_smem.ld)

  configure_file(
       $ENV{WELLSL4_BASE}/inc/linker/app_smem_aligned.ld
       ${PROJECT_BINARY_DIR}/inc/generated/app_smem_aligned.ld)

  configure_file(
       $ENV{WELLSL4_BASE}/inc/linker/app_smem_unaligned.ld
       ${PROJECT_BINARY_DIR}/inc/generated/app_smem_unaligned.ld)
endmacro()
