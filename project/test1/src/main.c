#include <sys/printk.h>
#include <kernel_object.h>
#include <kernel/stack.h>
#include <kernel/thread.h>
#include <user/anode.h>

#include <inc_l4/types.h>
#include <inc_l4/thread.h>
#include <inc_l4/ipc.h>
#include <inc_l4/message.h>
#include <inc_l4/schedule.h>

#include <drivers/stm32f4xx_hal_gpio.h>
#include <drivers/stm32f4xx_hal_rcc.h>

static THREAD_STACK_DEFINE(stack_1, 256);

void entry_1(void *p1, void *p2, void *p3);

static struct utcb virual_user_1;

static pager_context_t pager_1 = {
	.stack_ptr = stack_1,
	.stack_size = 256,
	.entry = entry_1,
	.p1 = (void *)0,
	.p2 = (void *)0,
	.p3 = (void *)0,
	.options = 1 << 2,
	.virual_user = &virual_user_1,
};

static L4_ThreadId_t main_id = {
	.raw = 2 << 14
};

static L4_ThreadId_t task1_id = {
	.raw = 256 << 14
};

static L4_ThreadId_t task1_space = {
	.raw = 2 << 14
};
	
static L4_ThreadId_t task1_sched = {
	.raw = 3 << 14
};
	
static L4_ThreadId_t task1_pager = {
	.raw = (word_t)&pager_1
};

static L4_Msg_t task1_msg;

static L4_Word_t task1_page[2] = {
	/* (0x10000000 & 0xFFFFFFC0) | 0xA,
	(0xFFFF & 0xFFFFFFF0) | 0x6 */
	(0x40021800 & 0xFFFFFFC0) | 0xA,
	(0x4000 & 0xFFFFFFF0) | 0x6
};

static L4_Word_t task1_page0[2] = {
	/* (0x10000000 & 0xFFFFFFC0) | 0xA,
	(0xFFFF & 0xFFFFFFF0) | 0x6 */
	(0x40023800 & 0xFFFFFFC0) | 0xA,
	(0x4000 & 0xFFFFFFF0) | 0x7
};

/*
static void _write(L4_Word_t *base, L4_Word_t len, L4_Word_t num)
{
	L4_Word_t *last = base + len;

	while (base < last)
		*base++ = num;
}*/


/* ENTRY_1 function is in a user mode state */
void entry_1(void *p1, void *p2, void *p3)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	L4_Word_t *gpiog = (L4_Word_t *)0x40021800;
	L4_Word_t flag = 0x0;

	/* GPIOG Periph clock enable */
	/* Configure PG13, PG14 in output pushpull mode */
	GPIO_InitStructure.Pin = 0x2000 | 0x4000;
	GPIO_InitStructure.Mode = 0x01;
	GPIO_InitStructure.Alternate = 0x00;
	GPIO_InitStructure.Speed = 0x03;
	GPIO_InitStructure.Pull = 0x00;

	__HAL_RCC_GPIOG_CLK_ENABLE();
	HAL_GPIO_Init((GPIO_TypeDef *)gpiog, &GPIO_InitStructure);
	/* TBD - Use GPIO, No Use UART */
	// printk("Hello World: WellsL4 OS entry 1!\r\n"); /* "Hello World: WellsL4 OS entry 1!\r\n" */

	for (;;)
	{
		/* TBD - IDLE Interrupt occur */
		HAL_GPIO_WritePin((GPIO_TypeDef *)gpiog, 0x2000, flag % 2);
		HAL_GPIO_WritePin((GPIO_TypeDef *)gpiog, 0x4000, flag % 2);
		flag++;
		L4_Yield();
	}
}

/* The MAIN function is in a pseudo-privileged mode state */
void main(void)
{
/*
	thread_control(256 << 14, 2, 3, (word_t)&pager_1);
	schedule_control(256 << 14, 200 << 20 | 1000 << 8 | 3, 0, 0x00 << 24 | 1 << 12 | 1, 0);
	exchange_ipc(256 << 14, 256 << 14, 0, (void *)0);
*/

	/* create task 1 */

	L4_ThreadControl(task1_id, task1_space, task1_sched, task1_pager, (void *)0);
	L4_Schedule(task1_id, 100 << 20 | 100 << 8 | 2, 0, 0xff << 24 | 1 << 12 | 1, 0, (L4_Word_t *)0);

	/* alloc task1 space and other resource */
	L4_MsgClear(&task1_msg);
	L4_MsgPut(&task1_msg, 0, 0, L4_NULL, 2, task1_page);
	L4_MsgLoad(&task1_msg);
	L4_SendReceive(main_id, task1_id);

	L4_MsgClear(&task1_msg);
	L4_MsgPut(&task1_msg, 0, 0, L4_NULL, 2, task1_page0);
	L4_MsgLoad(&task1_msg);
	L4_SendReceive(main_id, task1_id);

	// test
	L4_Word_t *usart1_dr = (L4_Word_t *)0x40011004;
	*usart1_dr = 0x12;

	/* exit main task */
	L4_Yield();
	for (;;)
	{
	}
}

