
#include <types_def.h>
#include <aarch32/cortex_m/cmse.h>
#include <sys/errno.h>

s32_t arm_cmse_mpu_region_get(u32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flag.mpu_region_valid)
	{
		return addr_info.flag.mpu_region;
	}

	return -EINVAL;
}

static s32_t arm_cmse_addr_read_write_ok(u32_t addr, s32_t force_npriv, s32_t rw)
{
	cmse_address_info_t addr_info;
	if (force_npriv)
	{
		addr_info = cmse_TTT((void *)addr);
	} 
	else 
	{
		addr_info = cmse_TT((void *)addr);
	}

	return rw ? addr_info.flag.readwrite_ok : addr_info.flag.read_ok;
}

s32_t arm_cmse_addr_read_ok(u32_t addr, s32_t force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 0);
}

s32_t arm_cmse_addr_readwrite_ok(u32_t addr, s32_t force_npriv)
{
	return arm_cmse_addr_read_write_ok(addr, force_npriv, 1);
}

static s32_t arm_cmse_addr_range_read_write_ok(u32_t addr, u32_t size,
	s32_t force_npriv, s32_t rw)
{
	s32_t flag = 0;

	if (force_npriv) 
	{
		flag |= CMSE_MPU_UNPRIV;
	}
	if (rw)
	{
		flag |= CMSE_MPU_READWRITE;
	} 
	else 
	{
		flag |= CMSE_MPU_READ;
	}
	
	if (cmse_check_address_range((void *)addr, size, flag) != NULL) {
		return 1;
	} 
	else 
	{
		return 0;
	}
}

s32_t arm_cmse_addr_range_read_ok(u32_t addr, u32_t size, s32_t force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 0);
}

s32_t arm_cmse_addr_range_readwrite_ok(u32_t addr, u32_t size, s32_t force_npriv)
{
	return arm_cmse_addr_range_read_write_ok(addr, size, force_npriv, 1);
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)

s32_t arm_cmse_mpu_nonsecure_region_get(u32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TTA((void *)addr);

	if (addr_info.flag.mpu_region_valid)
	{
		return  addr_info.flag.mpu_region;
	}

	return -EINVAL;
}

s32_t arm_cmse_sau_region_get(u32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flag.sau_region_valid)
	{
		return addr_info.flag.sau_region;
	}

	return -EINVAL;
}

s32_t arm_cmse_idau_region_get(u32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	if (addr_info.flag.idau_region_valid) 
	{
		return addr_info.flag.idau_region;
	}

	return -EINVAL;
}

s32_t arm_cmse_addr_is_secure(u32_t addr)
{
	cmse_address_info_t addr_info =	cmse_TT((void *)addr);

	return addr_info.flag.secure;
}

static s32_t arm_cmse_addr_nonsecure_read_write_ok(u32_t addr,
	s32_t force_npriv, s32_t rw)
{
	cmse_address_info_t addr_info;
	if (force_npriv)
	{
		addr_info = cmse_TTAT((void *)addr);
	} 
	else
	{
		addr_info = cmse_TTA((void *)addr);
	}

	return rw ? addr_info.flag.nonsecure_readwrite_ok :
		addr_info.flag.nonsecure_read_ok;
}

s32_t arm_cmse_addr_nonsecure_read_ok(u32_t addr, s32_t force_npriv)
{
	return arm_cmse_addr_nonsecure_read_write_ok(addr, force_npriv, 0);
}

s32_t arm_cmse_addr_nonsecure_readwrite_ok(u32_t addr, s32_t force_npriv)
{
	return arm_cmse_addr_nonsecure_read_write_ok(addr, force_npriv, 1);
}

static s32_t arm_cmse_addr_range_nonsecure_read_write_ok(u32_t addr, u32_t size,
	s32_t force_npriv, s32_t rw)
{
	s32_t flag = CMSE_NONSECURE;

	if (force_npriv) 
	{
		flag |= CMSE_MPU_UNPRIV;
	}
	if (rw)
	{
		flag |= CMSE_MPU_READWRITE;
	} 
	else 
	{
		flag |= CMSE_MPU_READ;
	}
	if (cmse_check_address_range((void *)addr, size, flag) != NULL) {
		return 1;
	}
	else
	{
		return 0;
	}
}

s32_t arm_cmse_addr_range_nonsecure_read_ok(u32_t addr, u32_t size,
	s32_t force_npriv)
{
	return arm_cmse_addr_range_nonsecure_read_write_ok(addr, size,
		force_npriv, 0);
}

s32_t arm_cmse_addr_range_nonsecure_readwrite_ok(u32_t addr, u32_t size,
	s32_t force_npriv)
{
	return arm_cmse_addr_range_nonsecure_read_write_ok(addr, size,
		force_npriv, 1);
}

#endif /* CONFIG_ARM_SECURE_FIRMWARE */
