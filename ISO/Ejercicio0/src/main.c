#include "board.h"
#include <stdlib.h>
#include <string.h>


#define STACK_SIZE  1024
#define DELAY_MS    1000

typedef void *( *task_type) (void *);


static void initHardware ();
//static void pausems(uint32_t t);
static void *task1 (void *param);
static void *task2 (void *param);


uint32_t    stack_1 [STACK_SIZE/4];
uint32_t    stack_2 [STACK_SIZE/4];

uint32_t    sp_1;
uint32_t    sp_2;

uint32_t    current_task;


static void initHardware ()
{
	Board_Init ();
	SystemCoreClockUpdate ();
	SysTick_Config (SystemCoreClock / 1000);
    // Minima prioridad a PendSV
    NVIC_SetPriority (PendSV_IRQn, (1 << __NVIC_PRIO_BITS) - 1);
}


static void schedule ()
{
    __ISB ();
    __DSB ();

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


void SysTick_Handler ()
{
    schedule ();
}


void *task1 (void *arg)
{
	while (1)
    {
        for (int i = 0; i < 500000; ++i);
        Board_LED_Toggle (LEDS_LED3);
		__WFI ();
	}

	return NULL;
}


void *task2 (void *arg)
{
	while (1)
    {
        for (int i = 0; i < 10000000; ++i);
        Board_LED_Toggle (LEDS_LED2);
		__WFI ();
	}

	return NULL;
}


void task_return_hook (void *ret_val)
{
	while (1)
    {
		__WFI ();
	}
}


uint32_t get_next_context (uint32_t current_sp)
{
	uint32_t next_sp;

	switch (current_task)
    {
		case 0:
			next_sp         = sp_1;
			current_task    = 1;
			break;

		case 1:
			sp_1            = current_sp;
			next_sp         = sp_2;
			current_task    = 2;
			break;

		case 2:
			sp_2            = current_sp;
			next_sp         = sp_1;
			current_task    = 1;
			break;

		default:
			while (1)
            {
				__WFI ();
			}
			break;
	}

	return next_sp;
}


void init_stack (uint32_t   stack[],
			 	 uint32_t   stack_size_bytes,
			 	 uint32_t   *sp,
		 		 task_type  entry_point,
				 void       *arg)
{
	memset (stack, 0, stack_size_bytes);

    const uint32_t stack_size_words = stack_size_bytes / 4;

	stack[stack_size_words - 1] = 1 << 24;                      // xPSR.T = 1
	stack[stack_size_words - 2] = (uint32_t)entry_point;		// xPC
	stack[stack_size_words - 3] = (uint32_t)task_return_hook;   // xLR
	stack[stack_size_words - 8] = (uint32_t)arg;				// R0
	stack[stack_size_words - 9] = 0xFFFFFFF9;					// LR IRQ

	// Tengo en cuenta los otros 8 registros basicos (words) no pusheados
    // al handlear una interrupcion
	*sp = (uint32_t) &stack[stack_size_words - 17];
}


int main ()
{
	init_stack (stack_1, STACK_SIZE, &sp_1, task1, (void *)0xBABABABA);
	init_stack (stack_2, STACK_SIZE, &sp_2, task2, (void *)0xCACACACA);

	initHardware ();

	while (1)
    {
		__WFI ();
	}

	return 0;
}
