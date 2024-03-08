#include <sys/assert.h>
#include <types_def.h>
#include <api/syscall.h>
#include <api/fatal.h>
/**
 * @brief Default character output routine that does nothing
 * @param c Character to swallow
 *
 * Note this is defined as a weak symbol, allowing architecture code
 * to override it where possible to enable very early logging.
 *
 * @return 0
 */
__weak sword_t arch_kprintf_char_out(sword_t c)
{
	ARG_UNUSED(c);
	return 0;
}

static sword_t (*arch_char_out)(sword_t) = arch_kprintf_char_out;

/**
 * @brief Install the character output routine for printk
 *
 * To be called by the platform's console driver at init time. Installs a
 * routine that outputs one ASCII character at a time.
 * @param fn putc routine to install
 *
 * @return N/A
 */
void arch_kprintf_set_hook(sword_t (*fn)(sword_t))
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
void *arch_kprintf_get_hook(void)
{
	return arch_char_out;
}

sword_t printf_c(const char chr)
{
	if(chr<0) return -1;
	if(chr == '\n') arch_char_out('\r');
	arch_char_out(chr);
	return 0;
}

sword_t printf_spaces(const sword_t n)
{
	sword_t m = -1;
	
	for(sword_t i = 0; i < n; i++) m = printf_c(' ');
	if(m == 0)
		return n;
	return -1;
}

sword_t printf_s(char *str)
{
	sword_t n = 0;
	sword_t m = -1;
	
	for(n = 0; *str; str++, n++) m = printf_c(*str);

	if(m == 0)
		return n;
	return -1;
}

static word_t xdiv(word_t x, word_t d)
{
    switch(d) 
	{
	    case 16:
	        return x / 16;
	    case 10:
	        return x / 10;
	    default:
	        return 0;
    }
}

static word_t xmod(word_t x, word_t d)
{
    switch(d) 
	{
	    case 16:
	        return x % 16;
	    case 10:
	        return x % 10;
	    default:
	        return 0;
    }
}

sword_t printf_unsigned_long(unsigned long x, word_t d)
{
	word_t i,j;
	//word_t d;
	char out[sizeof(word_t) * 2 + 3];
	
    /*
     * Only base 10 and 16 supported for now. We want to avoid invoking the
     * compiler's support libraries through doing arbitrary divisions.
     */	
	if(d != 10 || d != 16) return 0;

	if(x == 0)
	{
		printf_c('0');
	    return 1;
	}
	
	for (i = 0; x; x = xdiv(x,d), i++)
	{
		d = xmod(x,d);
		if(d >= 10) out[i] = 'a' + d - 10;
		else out[i] = '0' + d;
	}

	for(j = i; j > 0; j--) printf_c(out[j - 1]);

	return i;
}

sword_t printf_unsigned_long_long(unsigned long long x, word_t d)
{
    word_t upper, lower;
    word_t n = 0;
    word_t mask = 0xF0000000u;
    word_t shifts = 0;

    /* only implemented for hex, decimal is harder without 64 bit division */
    if (d != 16) {
        return 0;
    }

    /* we can't do 64 bit division so break it up into two hex numbers */
    upper = (word_t)(x >> 32llu);
    lower = (word_t) x & 0xffffffff;

    /* print first 32 bits if they exist */
    if (upper > 0) {
        n += printf_unsigned_long(upper, d);
        /* print leading 0s */
        while (!(mask & lower)) {
            printf_c('0');
            n++;
            mask = mask >> 4;
            shifts++;
            if (shifts == 8) {
                break;
            }
        }
    }
    /* print last 32 bits */
    n += printf_unsigned_long(lower, d);

    return n;
}

static FORCE_INLINE bool_t is_digit(char c)
{
    return c >= '0' &&
           c <= '9';
}

static FORCE_INLINE sword_t f_atoi(char c)
{
    return c - '0';
}

sword_t printf_core(const char *format, va_list ap)
{
	sword_t mode = 0;
	sword_t n = 0;
	sword_t m = 0;
	sword_t nspaces = 0;
    sword_t x = 0;
	unsigned long p = 0;
    long xx = 0;

	if(!format) return 0;
	
	for(;;)
	{
		if(!(*format)) break;

		if(!mode)
		{
			switch(*format)
			{
				case '%':
					mode = 1;
					format++;
					break;
				default:
					m = printf_c(*format);
					if(m==0) n++;
					format++;
					break;
			}
		}
		else
		{
			while(is_digit(*format)) 
			{
				nspaces = nspaces * 10 + f_atoi(*format);
				format++;
				if(format == NULL) break;
			}
			
			switch(*format)
			{
				case '%':
					m = printf_c('%');
					if(m==0) n++;
					format++;
					break;
				case 'd': case 'D':
					x = va_arg(ap,sword_t);
					
					if(x < 0)
					{
						m = printf_c('-');
						if(m == 0) n++;
						x = -x;
					}
					
					n += printf_unsigned_long(x, 10);
					format++;
					break;
				case 'u': case 'U':
					n += printf_unsigned_long(va_arg(ap,word_t), 10);
					format++;
					break;
				case 'x': case 'X':
					n += printf_unsigned_long(va_arg(ap,word_t), 16);
					format++;
					break;
				case 'p': case 't':
					p = va_arg(ap, word_t);
					
					if(p == 0) n += printf_s("(nil)");
					else
					{
						n += printf_s("0x");
						n += printf_unsigned_long(p,16);
					}
					
					format++;
					break;
				case 's':
					n += printf_s(va_arg(ap,char *));
					format++;
					break;
				case 'c':
					m = printf_c(va_arg(ap,sword_t));
					if(m == 0) n++;
					format++;
					break;
				case 'l': case 'L':
					format++;
					switch(*format)
					{
						case 'd':
							xx = va_arg(ap,long);
							if(xx < 0)
							{
								m = printf_c('-');
								if(m == 0) n++;
								xx = -xx;
							}

							n += printf_unsigned_long(xx, 10);
							format++;
							break;
						case 'l':
							if (*(format + 1) == 'x') 
							{
                        		n += printf_unsigned_long_long(va_arg(ap, unsigned long long), 16);
                    		}
                    		format += 2;
							break;
						case 'u':
							n += printf_unsigned_long(va_arg(ap, unsigned long), 10);
                    		format++;
							break;
						case 'x':
							n += printf_unsigned_long(va_arg(ap, unsigned long), 16);
                    		format++;
							break;
						default:
							return -1;
					}
					break;
				default:
					return -1;
			}
			n += printf_spaces(nspaces - n);
			nspaces = 0;
			mode = 0;
		}
	}

	return n;
}

sword_t vprintf(const char *format, va_list ap)
{
	va_list ap2;
	sword_t ret;
	
	va_copy(ap2, ap);
	if (printf_core(format, ap2) < 0) {
		va_end(ap2);
		return -1;
	}
	
	ret = printf_core(format, ap2);
	va_end(ap2);
	return ret;
}

sword_t kputs(const char *s)
{
	for(;*s;s++) printf_c(*s);
	printf_c('\n');
	return 0;
}

sword_t kprintf(const char *format, ...)
{
	sword_t ret;
	va_list ap;
	va_start(ap, format);
	ret = vprintf(format, ap);
	va_end(ap);
	return ret;
}

/**
 *
 * @brief Assert Action Handler
 *
 * This routine implements the action to be taken when an assertion fails.
 *
 * System designers may wish to substitute this implementation to take other
 * actions, such as logging program counter, line number, debug information
 * to a persistent repository and/or rebooting the system.
 *
 * @param N/A
 *
 * @return N/A
 */

__weak void assert_post_action(void)
{
#ifdef CONFIG_USERSPACE
	/* User record_threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
	if (is_user_context()) 
	{
		k_oops();
	}
#endif

	k_panic();
}

void __NORETURN __NO_INLINE __VISIBLE halt(void)
{
    /* halt is actually, idle thread without the interrupts */
    asm volatile("cpsid i");
    kprintf("halting...");
    while(1)
	{
		/*tickless or wait*/
		__asm volatile ("wfi");
	}
    UNREACHABLE();
}

void _fail(
    const char  *s,
    const char  *file,
    word_t line,
    const char  *function)
{
    kprintf(
        "L4 called fail at %s:%u in function %s, saying \"%s\"\n",
        file,
        line,
        function,
        s
    );
    halt();
}

void _assert_fail(
    const char  *assertion,
    const char  *file,
    word_t line,
    const char  *function)
{
    kprintf("L4 failed assertion '%s' at %s:%u in function %s\n",
           assertion,
           file,
           line,
           function
          );
    halt();
}
