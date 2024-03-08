# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__WELLSL4_SUPERVISOR__)

if(CONFIG_GEN_ISR_TABLES)
  wellsl4_library()

  wellsl4_library_sources_ifdef(
          CONFIG_GEN_ISR_TABLES
          isr_tables.c
  )
  wellsl4_library_sources(
          registers.c
  )
endif()

# Put functions and data in their own binary sections so that ld can
# garbage collect them
wellsl4_cc_option(-ffunction-sections -fdata-sections)

wellsl4_linker_sources_ifdef(CONFIG_GEN_ISR_TABLES
        SECTIONS
        isr_tables.ld
        )

wellsl4_linker_sources_ifdef(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
        RAM_SECTIONS
        ramfunc.ld
        )

wellsl4_linker_sources_ifdef(CONFIG_NOCACHE_MEMORY
        RAM_SECTIONS
        nocache.ld
        )

# Only ARM, X86 and OPENISA_RV32M1_RISCV32 use TEXT_SECTION_OFFSET.
if (DEFINED CONFIG_ARM OR DEFINED CONFIG_X86
        OR DEFINED CONFIG_SOC_OPENISA_RV32M1_RISCV32)
  wellsl4_linker_sources_ifdef(TEXT_START SORT_KEY 0x0 text_section_offset.ld)
endif()

include_directories(
        ${WELLSL4_BASE}/inc
        ${WELLSL4_BASE}/src/arch
)

add_subdirectory(${WELLSL4_BASE}/src/arch/${ARCH})