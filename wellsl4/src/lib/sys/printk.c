/**
 * @file
 * @brief Low-level debug output
 *
 * Low-level debugging output. Platform installs a character output routine at
 * init time. If no routine is installed, a nop routine is called.
 */


#include <sys/printk.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sys/util.h>

typedef sword_t (*out_handler_t)(sword_t c, void *ctx);

enum pad_type {
	pad_none_pad,
	pad_zero_before_pad,
	pad_space_before_pad,
	pad_space_after_pad,
};

struct char_out_context {
	sword_t count;
};

struct str_context {
	char *str;
	sword_t max;
	sword_t count;
};

#ifdef CONFIG_USERSPACE
struct buf_out_context {
	sword_t count;
	word_t buf_count;
	char buf[CONFIG_PRINTBUFFER_SIZE];
};
#endif

/**
 * @brief Default character output routine that does nothing
 * @param c Character to swallow
 *
 * Note this is defined as a weak symbol, allowing architecture code
 * to override it where possible to enable very early logging.
 *
 * @return 0
 */
__weak sword_t arch_printk_char_out(sword_t c)
{
	ARG_UNUSED(c);
	return 0;
}

sword_t (*arch_char_out)(sword_t) = arch_printk_char_out;

/**
 * @brief Install the character output routine for printk
 *
 * To be called by the platform's console driver at init time. Installs a
 * routine that outputs one ASCII character at a time.
 * @param fn putc routine to install
 *
 * @return N/A
 */
void arch_printk_set_hook(sword_t (*fn)(sword_t))
{
	arch_char_out = fn;
}

/**
 * @brief Get the _current_thread character output routine for printk
 *
 * To be called by any console driver that would like to save
 * _current_thread hook - if any - for later re-installation.
 *
 * @return a function pointer or NULL if no hook is set
 */
void *arch_printk_get_hook(void)
{
	return arch_char_out;
}

static sword_t printk_char_out(sword_t c, void *ctx_p)
{
	struct char_out_context *ctx = ctx_p;

	ctx->count++;
	return arch_char_out(c);
}


static sword_t printk_string(sword_t c, struct str_context *ctx)
{
	if (ctx->str == NULL || ctx->count >= ctx->max)
	{
		ctx->count++;
		return c;
	}

	if (ctx->count == ctx->max - 1) 
	{
		ctx->str[ctx->count++] = '\0';
	} 
	else
	{
		ctx->str[ctx->count++] = c;
	}

	return c;
}

static void printk_error(out_handler_t out, void *ctx)
{
	out('E', ctx);
	out('R', ctx);
	out('R', ctx);
}

/**
 * @brief Output an unsigned long long in hex format
 *
 * Output an unsigned long long on output installed by platform at init time.
 * Able to print full 64-bit values.
 * @param num Number to output
 *
 * @return N/A
 */
static void printk_hex_ulong(out_handler_t out, void *ctx,
			      const unsigned long long num,
			      enum pad_type padding,
			      sword_t min_width)
{
	sword_t shift = sizeof(num) * 8;
	sword_t found_largest_digit = 0;
	sword_t remaining = 16; /* 16 digits max */
	sword_t digits = 0;
	char    nibble;

	while (shift >= 4) 
	{
		shift -= 4;
		nibble = (num >> shift) & 0xf;

		if (nibble != 0 || found_largest_digit != 0 || shift == 0) 
		{
			found_largest_digit = 1;
			nibble += nibble > 9 ? 87 : 48;
			out((sword_t)nibble, ctx);
			digits++;
			continue;
		}

		if (remaining-- <= min_width)
		{
			if (padding == pad_zero_before_pad)
			{
				out('0', ctx);
			} 
			else if (padding == pad_space_before_pad) 
			{
				out(' ', ctx);
			}
		}
	}

	if (padding == pad_space_after_pad)
	{
		remaining = min_width * 2 - digits;
		while (remaining-- > 0)
		{
			out(' ', ctx);
		}
	}
}

/**
 * @brief Output an unsigned long in decimal format
 *
 * Output an unsigned long on output installed by platform at init time.
 *
 * @param num Number to output
 *
 * @return N/A
 */
static void printk_dec_ulong(out_handler_t out, void *ctx,
			      const unsigned long num, enum pad_type padding,
			      sword_t min_width)
{
	unsigned long pos = 1000000000;
	unsigned long remainder = num;
	sword_t found_largest_digit = 0;
	sword_t remaining = sizeof(long) * 5 / 2;
	sword_t digits = 1;

	if (sizeof(long) == 8)
	{
		pos *= 10000000000;
	}

	/* make sure we don't skip if value is zero */
	if (min_width <= 0)
	{
		min_width = 1;
	}

	while (pos >= 10)
	{
		if (found_largest_digit != 0 || remainder >= pos)
		{
			found_largest_digit = 1;
			out((sword_t)(remainder / pos + 48), ctx);
			digits++;
		} 
		else if (remaining <= min_width && padding < pad_space_after_pad)
		{
			out((sword_t)(padding == pad_zero_before_pad ? '0' : ' '), ctx);
			digits++;
		}
		
		remaining--;
		remainder %= pos;
		pos /= 10;
	}
	
	out((sword_t)(remainder + 48), ctx);

	if (padding == pad_space_after_pad) 
	{
		remaining = min_width - digits;
		while (remaining-- > 0)
		{
			out(' ', ctx);
		}
	}
}


/**
 * @brief Printk internals
 *
 * See printk() for description.
 * @param fmt Format string
 * @param ap Variable parameters
 *
 * @return N/A
 */
void vprintk_core(out_handler_t out, void *ctx, const char *fmt, va_list ap)
{
	sword_t might_format = 0; /* 1 if encountered a '%' */
	enum pad_type padding = pad_none_pad;
	sword_t min_width = -1;
	char length_mod = 0;

	/* fmt has already been adjusted if needed */

	while (*fmt) 
	{
		if (!might_format)
		{
			if (*fmt != '%')
			{
				out((sword_t)*fmt, ctx);
			} 
			else
			{
				might_format = 1;
				min_width = -1;
				padding = pad_none_pad;
				length_mod = 0;
			}
		} 
		else
		{
			switch (*fmt) 
			{
				case '-':
					padding = pad_space_after_pad;
					goto next_format;
				case '0':
					if (min_width < 0 && padding == pad_none_pad) 
					{
						padding = pad_zero_before_pad;
						goto next_format;
					}
					/* Fall through */
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					/* Fall through */
				case '9':
					if (min_width < 0) 
					{
						min_width = *fmt - '0';
					} 
					else
					{
						min_width = 10 * min_width + *fmt - '0';
					}

					if (padding == pad_none_pad)
					{
						padding = pad_space_before_pad;
					}
					goto next_format;
				case 'h':
				case 'l':
				case 'z':
					if (*fmt == 'h' && length_mod == 'h') 
					{
						length_mod = 'H';
					} 
					else if (*fmt == 'l' && length_mod == 'l')
					{
						length_mod = 'L';
					} 
					else if (length_mod == 0)
					{
						length_mod = *fmt;
					} 
					else
					{
						out((sword_t)'%', ctx);
						out((sword_t)*fmt, ctx);
						break;
					}
					goto next_format;
				case 'd':
				case 'i': {
					long d;

					if (length_mod == 'z') 
					{
						d = va_arg(ap, ssize_t);
					} 
					else if (length_mod == 'l') 
					{
						d = va_arg(ap, long);
					}
					else if (length_mod == 'L') 
					{
						long long lld = va_arg(ap, long long);
						if (lld > __LONG_MAX__ || lld < ~__LONG_MAX__) 
						{
							printk_error(out, ctx);
							break;
						}
						d = lld;
					} 
					else 
					{
						d = va_arg(ap, sword_t);
					}

					if (d < 0)
					{
						out((sword_t)'-', ctx);
						d = -d;
						min_width--;
					}
					printk_dec_ulong(out, ctx, d, padding, min_width);
					break;
				}
				case 'u': {
					unsigned long u;

					if (length_mod == 'z') 
					{
						u = va_arg(ap, size_t);
					}
					else if (length_mod == 'l') 
					{
						u = va_arg(ap, unsigned long);
					} 
					else if (length_mod == 'L')
					{
						unsigned long long llu = va_arg(ap, unsigned long long);
						if (llu > ~0UL) 
						{
							printk_error(out, ctx);
							break;
						}
						u = llu;
					} 
					else 
					{
						u = va_arg(ap, word_t);
					}

					printk_dec_ulong(out, ctx, u, padding, min_width);
					break;
				}
				case 'p':
					out('0', ctx);
					out('x', ctx);
					/* left-pad pointers with zeros */
					padding = pad_zero_before_pad;
					if (IS_ENABLED(CONFIG_64BIT))
					{
						min_width = 16;
					} 
					else 
					{
						min_width = 8;
					}
					/* Fall through */
				case 'x':
				case 'X': {
					unsigned long long x;

					if (*fmt == 'p')
					{
						x = (uintptr_t)va_arg(ap, void *);
					} 
					else if (length_mod == 'l') 
					{
						x = va_arg(ap, unsigned long);
					} 
					else if (length_mod == 'L')
					{
						x = va_arg(ap, unsigned long long);
					} 
					else 
					{
						x = va_arg(ap, word_t);
					}

					printk_hex_ulong(out, ctx, x, padding, min_width);
					break;
				}
				case 's': {
					char *s = va_arg(ap, char *);
					char *start = s;

					while (*s) 
					{
						out((sword_t)(*s++), ctx);
					}

					if (padding == pad_space_after_pad)
					{
						sword_t remaining = min_width - (s - start);
						while (remaining-- > 0) 
						{
							out(' ', ctx);
						}
					}
					break;
				}
				case 'c': {
					sword_t c = va_arg(ap, sword_t);

					out(c, ctx);
					break;
				}
				case '%': {
					out((sword_t)'%', ctx);
					break;
				}
				default:
					out((sword_t)'%', ctx);
					out((sword_t)*fmt, ctx);
					break;
			}
			might_format = 0;
		}
next_format:
		++fmt;
	}
}


void printk_string_out(char *c, size_t n)
{
	sword_t i;

	for (i = 0; i < n; i++) 
	{
		arch_char_out(c[i]);
	}
}

#ifdef CONFIG_USERSPACE
#include <api/syscall.h>
#include <syscalls/printk.h>
void syscall_uprintk_string_out(char *c, size_t n)
{
	printk_string_out((char *)c, n);
}

static void printk_buf_flush(struct buf_out_context *ctx)
{
	uprintk_string_out(ctx->buf, ctx->buf_count);
	ctx->buf_count = 0U;
}

static sword_t printk_buf_char_out(sword_t c, void *ctx_p)
{
	struct buf_out_context *ctx = ctx_p;

	ctx->count++;
	ctx->buf[ctx->buf_count++] = c;
	
	if (ctx->buf_count == CONFIG_PRINTBUFFER_SIZE) 
	{
		printk_buf_flush(ctx);
	}

	return c;
}

void vprintk(const char *fmt, va_list ap)
{
	if (is_user_context()) 
	{
		struct buf_out_context ctx = { 0 };

		vprintk_core(printk_buf_char_out, &ctx, fmt, ap);

		if (ctx.buf_count)
		{
			printk_buf_flush(&ctx);
		}
	} 
	else 
	{
		struct char_out_context ctx = { 0 };

		vprintk_core(printk_char_out, &ctx, fmt, ap);
	}
}

#else

void vprintk(const char *fmt, va_list ap)
{
	struct char_out_context ctx = { 0 };

	vprintk_core(printk_char_out, &ctx, fmt, ap);
}

#endif

/**
 * @brief Output a string
 *
 * Output a string on output installed by platform at init time. Some
 * printf-like formatting is available.
 *
 * Available formatting:
 * - %x/%X:  outputs a number in hexadecimal format
 * - %s:     outputs a null-terminated string
 * - %p:     pointer, same as %x with a 0x prefix
 * - %u:     outputs a number in unsigned decimal format
 * - %d/%i:  outputs a number in signed decimal format
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 *
 * @param fmt formatted string to output
 *
 * @return N/A
 */
void printk(const char *fmt, ...) /* const */
{
	va_list ap;
	
	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

sword_t vsnprintk(char *str, size_t size, const char *fmt, va_list ap)
{
	struct str_context ctx = { str, size, 0 };

	vprintk_core((out_handler_t)printk_string, &ctx, fmt, ap);

	if (ctx.count < ctx.max)
	{
		str[ctx.count] = '\0';
	}

	return ctx.count;
}

sword_t snprintk(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	sword_t ret;

	va_start(ap, fmt);
	ret = vsnprintk(str, size, fmt, ap);
	va_end(ap);

	return ret;
}
