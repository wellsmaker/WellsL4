#include <types_def.h>
#include <stm32f4xx_hal_uart.h>

extern UART_HandleTypeDef UartHandle;
static sword_t debug_block_out(sword_t str)
{
	sword_t status;
	status = (sword_t)HAL_UART_Transmit(&UartHandle, (byte_t *)&str, 1, HAL_MAX_DELAY);
	return status;
}

void init_serial_object(void)
{
	extern void arch_printk_set_hook(sword_t (*fn)(sword_t));
	extern s32_t UARTInit(void);
	extern s32_t SystemClockInit(void);

	SystemClockInit();
	UARTInit();
	__HAL_UART_CLEAR_FLAG(&UartHandle, UART_FLAG_TXE);
	arch_printk_set_hook(debug_block_out);
}
