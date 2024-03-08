/* SPDX-License-Identifier: Apache-2.0 */

	SECTION_DATA_PROLOGUE(initlevel,,)
	{
		DEVICE_INIT_SECTIONS()
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_GEN_ISR_TABLES) && defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_DATA_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(SW_ISR_TABLE)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

	/* verify we don't have rogue .init_<something> initlevel sections */
	SECTION_DATA_PROLOGUE(initlevel_error,,)
	{
		DEVICE_INIT_UNDEFINED_SECTION()
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

#ifdef CONFIG_USERSPACE
	/* All kernel objects within are assumed to be either completely
	 * initialized at build time, or initialized automatically at runtime
	 * via iteration before the POST_KERNEL phase.
	 *
	 * These two symbols only used by gen_kobject_list.py
	 */

	_static_kernel_objects_begin = .;
#endif /* CONFIG_USERSPACE */

	SECTION_DATA_PROLOGUE(_k_mem_pool_area,,SUBALIGN(4))
	{
		_k_mem_pool_list_start = .;
		KEEP(*("._k_mem_pool.static.*"))
		_k_mem_pool_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
	
#ifdef CONFIG_USERSPACE
	_static_kernel_objects_end = .;
#endif