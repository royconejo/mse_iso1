#include "os.h"
#include "systick.h"
#include "board.h"


void HardFault_Handler()
{
    while (1)
    {
        for (int i = 0; i < 5000000; ++i);
        Board_LED_Toggle (LEDS_LED1);
    }
}


OS_TaskRetVal task1 (OS_TaskParam arg)
{
    int x = 20;

	while (x--)
    {
        Board_LED_Toggle (LEDS_LED3);
   //     for (int i = 0; i < 5000000; ++i);
        enum OS_Result r = OS_TaskPeriodicDelay (500);
	}

    OS_Terminate ();

	return 0;
}


OS_TaskRetVal task2 (OS_TaskParam arg)
{
    int x = 2;

	while (x--)
    {
        Board_LED_Toggle (LEDS_LED2);
    //    for (int i = 0; i < 10000000; ++i);
        enum OS_Result r = OS_TaskPeriodicDelay (1000);
	}

	return 0xFFFFFFFF;
}

/*
uint8_t task1Buffer [OS_MinTaskBufferSize ()];
uint8_t task2Buffer [OS_MinTaskBufferSize ()];
*/


uint8_t task1Buffer[1024];
uint8_t task2Buffer[1024];


OS_TaskRetVal boot (OS_TaskParam arg)
{
    OS_TaskStart (task1Buffer, sizeof(task1Buffer), task1, NULL,
                  OS_TaskPriority_Drv1, "T1");

    OS_TaskStart (task2Buffer, sizeof(task2Buffer), task2, NULL,
                  OS_TaskPriority_Drv1, "T2");

    return 0;
}


int main ()
{
    Board_Init ();
	SystemCoreClockUpdate ();
	SYSTICK_SetMillisecondPeriod (1);

    uint8_t initBuffer [OS_InitBufferSize ()];

    OS_Init (initBuffer);
    OS_Start (boot);

    Board_LED_Toggle (LEDS_LED1);

	return 0;
}
