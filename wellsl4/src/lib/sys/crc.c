#include <sys/crc.h>

byte_t crc7_be(byte_t seed, const byte_t *src, size_t len)
{
	while (len--) 
	{
		byte_t e = seed ^ *src++;
		byte_t f = e ^ (e >> 4) ^ (e >> 7);

		seed = (f << 1) ^ (f << 4);
	}

	return seed;
}

static const byte_t crc8_ccitt_small_table[16] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d
};

byte_t crc8_ccitt(byte_t val, const void *buf, size_t cnt)
{
	sword_t i;
	const byte_t *p = buf;

	for (i = 0; i < cnt; i++)
	{
		val ^= p[i];
		val = (val << 4) ^ crc8_ccitt_small_table[val >> 4];
		val = (val << 4) ^ crc8_ccitt_small_table[val >> 4];
	}
	return val;
}

u16_t crc16(const byte_t *src, size_t len, u16_t polynomial,
	    u16_t initial_value, bool pad)
{
	u16_t crc = initial_value;
	size_t padding = pad ? sizeof(crc) : 0;
	size_t i, b;

	/* src length + padding (if required) */
	for (i = 0; i < len + padding; i++)
	{
		for (b = 0; b < 8; b++) 
		{
			u16_t divide = crc & 0x8000UL;

			crc = (crc << 1U);

			/* choose input bytes or implicit trailing zeros */
			if (i < len) 
			{
				crc |= !!(src[i] & (0x80U >> b));
			}

			if (divide != 0U) 
			{
				crc = crc ^ polynomial;
			}
		}
	}

	return crc;
}

u16_t crc16_ccitt(u16_t seed, const byte_t *src, size_t len)
{
	for (; len > 0; len--) 
	{
		byte_t e, f;

		e = seed ^ *src++;
		f = e ^ (e << 4);
		seed = (seed >> 8) ^ (f << 8) ^ (f << 3) ^ (f >> 4);
	}

	return seed;
}

u16_t crc16_itu_t(u16_t seed, const byte_t *src, size_t len)
{
	for (; len > 0; len--) 
	{
		seed = (seed >> 8U) | (seed << 8U);
		seed ^= *src++;
		seed ^= (seed & 0xffU) >> 4U;
		seed ^= seed << 12U;
		seed ^= (seed & 0xffU) << 5U;
	}

	return seed;
}

u32_t crc32_ieee(const byte_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

u32_t crc32_ieee_update(u32_t crc, const byte_t *data, size_t len)
{
	crc = ~crc;
	for (size_t i = 0; i < len; i++)
	{
		crc = crc ^ data[i];

		for (byte_t j = 0; j < 8; j++)
		{
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
		}
	}

	return (~crc);
}
