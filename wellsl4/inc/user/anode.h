/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APP_MEMORY_APP_MEMDOMAIN_H_
#define APP_MEMORY_APP_MEMDOMAIN_H_

#include <linker/linker_defs.h>



#ifdef CONFIG_USERSPACE

/**
 * @brief Name of the data section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define APP_DMEM_SECTION(id) data_smem_##id##_data

/**
 * @brief Name of the bss section for a particular partition
 *
 * Useful for defining memory pools, or any other macro that takes a
 * section name as a parameter.
 *
 * @param id Partition name
 */
#define APP_BMEM_SECTION(id) data_smem_##id##_bss

/**
 * @brief Place data in a partition's data section
 *
 * Globals tagged with this will end up in the data section for the
 * specified memory partition. This data should be initialized to some
 * desired value.
 *
 * @param id Name of the memory partition to associate this data
 */
#define APP_DMEM(id) GENERIC_SECTION(APP_DMEM_SECTION(id))

/**
 * @brief Place data in a partition's bss section
 *
 * Globals tagged with this will end up in the bss section for the
 * specified memory partition. This data will be zeroed at boot.
 *
 * @param id Name of the memory partition to associate this data
 */
#define APP_BMEM(id) GENERIC_SECTION(APP_BMEM_SECTION(id))

struct app_region {
	void *bss_start;
	size_t bss_size;
};

#define APP_START(id) data_smem_##id##_part_start
#define APP_SIZE(id) data_smem_##id##_part_size
#define APP_BSS_START(id) data_smem_##id##_bss_start
#define APP_BSS_SIZE(id) data_smem_##id##_bss_size

/* If a partition is declared with APPMEM_PARTITION, but never has any
 * data assigned to its contents, then no symbols with its prefix will end
 * up in the symbol table. This prevents gen_app_partitions.py from detecting
 * that the partition exists, and the linker symbols which specify partition
 * bounds will not be generated, resulting in build errors.
 *
 * What this FORCE_INLINE assembly code does is define a symbol with no data.
 * This should work for all arches that produce ELF binaries, see
 * https://sourceware.org/binutils/docs/as/Section.html
 *
 * We don't know what active flag/type of the pushed section were, so we are
 * specific: "aw" indicates section is allocatable and writable,
 * and "@progbits" indicates the section has data.
 */
#ifdef CONFIG_ARM
/* ARM has a quirk in that '@' denotes a comment, so we have to send
 * %progbits to the assembler instead.
 */
#define PROGBITS_SYM	"\%"
#else
#define PROGBITS_SYM "@"
#endif

#define APPMEM_PLACEHOLDER(name) \
	__asm__ ( \
		".pushsection " STRINGIFY(APP_DMEM_SECTION(name)) \
			",\"aw\"," PROGBITS_SYM "progbits\n\t" \
		".global " STRINGIFY(name) "_placeholder\n\t" \
		STRINGIFY(name) "_placeholder:\n\t" \
		".popsection\n\t")

/**
 * @brief Define an application memory partition with linker support
 *
 * Defines a k_mem_paritition with the provided name.
 * This name may be used with the APP_DMEM and APP_BMEM macros to
 * place globals automatically in this partition.
 *
 * NOTE: placeholder char variable is defined here to prevent build errors
 * if a partition is defined but nothing ever placed in it.
 *
 * @param name Name of the partition to declare
 */
#define APPMEM_PARTITION_DEFINE(name) \
	extern char APP_START(name)[]; \
	extern char APP_SIZE(name)[]; \
	partition_t name = { \
		.start = (uintptr_t) &APP_START(name), \
		.size = (size_t) &APP_SIZE(name), \
		.attr = MEM_PARTITION_P_RW_U_RW \
	}; \
	extern char APP_BSS_START(name)[]; \
	extern char APP_BSS_SIZE(name)[]; \
	GENERIC_SECTION(.app_regions.name) \
	struct app_region name##_region = { \
		.bss_start = &APP_BSS_START(name), \
		.bss_size = (size_t) &APP_BSS_SIZE(name) \
	}; \
	APPMEM_PLACEHOLDER(name);
#else

#define APP_BMEM(ptn)
#define APP_DMEM(ptn)
#define APP_DMEM_SECTION(ptn) .data
#define APP_BMEM_SECTION(ptn) .bss
#define APPMEM_PARTITION_DEFINE(name)

#endif
#endif
