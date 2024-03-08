# SPDX-License-Identifier: Apache-2.0

wellsl4_library()

wellsl4_library_sources(                             arm_mpu_anode.c)
wellsl4_library_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU arm_mpu.c)


if(CONFIG_ARM_MPU AND NOT CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)
  wellsl4_library_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU
    arm_mpu_regions.c
    )
endif()

wellsl4_library_include_directories(
  ${WELLSL4_BASE}/inc/arch/arm/aarch32/cortex_m
)
