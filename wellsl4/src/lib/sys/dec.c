#include <sys/util.h>

byte_t u8_to_dec(char *buf, byte_t buflen, byte_t value)
{
	byte_t divisor = 100;
	byte_t num_digits = 0;
	byte_t digit;

	while (buflen > 0 && divisor > 0) 
	{
		digit = value / divisor;
		if (digit != 0 || divisor == 1 || num_digits != 0) 
		{
			*buf = (char)digit + '0';
			buf++;
			buflen--;
			num_digits++;
		}

		value -= digit * divisor;
		divisor /= 10;
	}

	if (buflen)
	{
		*buf = '\0';
	}

	return num_digits;
}
