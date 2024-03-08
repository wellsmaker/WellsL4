

#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <aarch32/cortex_m/tz.h>
#include <aarch32/cortex_m/exc.h>

static void configure_nonsecure_vtor_offset(u32_t vtor_ns)
{
	SCB_NS->VTOR = vtor_ns;
}

static void configure_nonsecure_msp(u32_t msp_ns)
{
	__Tset_MSP_NS(msp_ns);
}

static void configure_nonsecure_psp(u32_t psp_ns)
{
	__Tset_PSP_NS(psp_ns);
}

static void configure_nonsecure_control(u32_t spsel_ns, u32_t npriv_ns)
{
	u32_t control_ns = __Tget_CONTROL_NS();

	/* Only nPRIV and SPSEL bits are banked between security states. */
	control_ns &= ~(CONTROL_SPSEL_Msk | CONTROL_nPRIV_Msk);

	if (spsel_ns)
	{
		control_ns |= CONTROL_SPSEL_Msk;
	}
	if (npriv_ns) 
	{
		control_ns |= CONTROL_nPRIV_Msk;
	}

	__Tset_CONTROL_NS(control_ns);
}

#if defined(CONFIG_ARMV8_M_MAINLINE)

/* Only ARMv8-M Mainline implementations have Non-Secure instances of
 * Stack Pointer Limit registers.
 */

void tnonsecure_msplim_set(u32_t val)
{
	__Tset_MSPLIM_NS(val);
}

void tnonsecure_psplim_set(u32_t val)
{
	__Tset_PSPLIM_NS(val);
}
#endif /* CONFIG_ARMV8_M_MAINLINE */

void tnonsecure_state_setup(const tnonsecure_setup_conf_t *p_ns_conf)
{
	configure_nonsecure_vtor_offset(p_ns_conf->vtor_ns);
	configure_nonsecure_msp(p_ns_conf->msp_ns);
	configure_nonsecure_psp(p_ns_conf->psp_ns);
	/* Select which stack-pointer to use (MSP or PSP) and
	 * the privilege level for thread mode.
	 */
	configure_nonsecure_control(p_ns_conf->control_ns.spsel,
		p_ns_conf->control_ns.npriv);
}

void tnbanked_exception_target_state_set(sword_t secure_state)
{
	u32_t aircr_payload = SCB->AIRCR & (~(SCB_AIRCR_VECTKEY_Msk));
	if (secure_state)
	{
		aircr_payload &= ~(SCB_AIRCR_BFHFNMINS_Msk);
	} 
	else 
	{
		aircr_payload |= SCB_AIRCR_BFHFNMINS_Msk;
	}
	
	SCB->AIRCR = ((AIRCR_VECT_KEY_PERMIT_WRITE << SCB_AIRCR_VECTKEY_Pos)
			& SCB_AIRCR_VECTKEY_Msk)
		| aircr_payload;
}

void tnonsecure_exception_prio_config(sword_t secure_boost)
{
	u32_t aircr_payload = SCB->AIRCR & (~(SCB_AIRCR_VECTKEY_Msk));
	if (secure_boost)
	{
		aircr_payload |= SCB_AIRCR_PRIS_Msk;
	} 
	else
	{
		aircr_payload &= ~(SCB_AIRCR_PRIS_Msk);
	}
	SCB->AIRCR = ((AIRCR_VECT_KEY_PERMIT_WRITE << SCB_AIRCR_VECTKEY_Pos)
			& SCB_AIRCR_VECTKEY_Msk)
		| aircr_payload;
}

void tnonsecure_system_reset_req_block(sword_t block)
{
	u32_t aircr_payload = SCB->AIRCR & (~(SCB_AIRCR_VECTKEY_Msk));
	if (block) 
	{
		aircr_payload |= SCB_AIRCR_SYSRESETREQS_Msk;
	} 
	else
	{
		aircr_payload &= ~(SCB_AIRCR_SYSRESETREQS_Msk);
	}
	SCB->AIRCR = ((0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
			& SCB_AIRCR_VECTKEY_Msk)
		| aircr_payload;
}

#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
void tnonsecure_fpu_access_enable(void)
{
	SCB->NSACR |=
		(1UL << SCB_NSACR_CP10_Pos) | (1UL << SCB_NSACR_CP11_Pos);
}
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */

void tsau_configure(sword_t enable, sword_t allns)
{
	if (enable) 
	{
		TSAU_Enable();
	}
	else
	{
		TSAU_Disable();
		if (allns) 
		{
			SAU->CTRL |= SAU_CTRL_ALLNS_Msk;
		}
		else 
		{
			SAU->CTRL &= ~(SAU_CTRL_ALLNS_Msk);
		}
	}
}

u32_t tsau_number_of_regions_get(void)
{
	return SAU->TYPE & SAU_TYPE_SREGION_Msk;
}

#if defined(CONFIG_CPU_HAS_ARM_SAU)
#if defined (__SAUREGION_PRESENT) && (__SAUREGION_PRESENT == 1U)
sword_t tsau_region_configure_enable(tsau_conf_t *p_sau_conf)
{
	u32_t regions = tsau_number_of_regions_get();

	if ((p_sau_conf->region_num == 0) ||
		(p_sau_conf->region_num > (regions - 1)))
	{
		return 0;
	}

	/* Valid region */
	SAU->RNR = p_sau_conf->region_num & SAU_RNR_REGION_Msk;

	if (p_sau_conf->enable)
	{
		SAU->RLAR = SAU_RLAR_ENABLE_Msk
			| (SAU_RLAR_LADDR_Msk & p_sau_conf->limit_addr)
			| (p_sau_conf->nsc ? SAU_RLAR_NSC_Msk : 0);
		SAU->RBAR = p_sau_conf->base_addr & SAU_RBAR_BADDR_Msk;
	} 
	else 
	{
		SAU->RLAR &= ~(SAU_RLAR_ENABLE_Msk);
	}

	return 1;
}
#else
#error "ARM SAU not implemented"
#endif
#endif /* CONFIG_CPU_HAS_ARM_SAU */
