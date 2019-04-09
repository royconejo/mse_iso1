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


OS_TaskRet task1 (OS_TaskParam arg)
{
    int x = 20;

	while (x--)
    {
        Board_LED_Toggle (LEDS_LED3);
   //     for (int i = 0; i < 5000000; ++i);
        OS_TaskPeriodicDelay (500);
	}

    OS_Terminate ();

	OS_TaskReturn (0);
}


OS_TaskRet task2 (OS_TaskParam arg)
{
    int x = 2;

	while (x--)
    {
        Board_LED_Toggle (LEDS_LED2);
    //    for (int i = 0; i < 10000000; ++i);
        OS_TaskPeriodicDelay (1000);
	}

	OS_TaskReturn (0xFFFFFFFF);
}


int main ()
{
    Board_Init ();
	SystemCoreClockUpdate ();
	SYSTICK_SetMillisecondPeriod (1);

    uint8_t initBuffer  [OS_InitBufferSize    ()];
    uint8_t task1Buffer [OS_MinTaskBufferSize ()];
    uint8_t task2Buffer [OS_MinTaskBufferSize ()];

    OS_Init (initBuffer);

    OS_TaskStart (task1Buffer, sizeof(task1Buffer), task1, NULL,
                  OS_TaskPriorityLevel3_App, "T1");
    OS_TaskStart (task2Buffer, sizeof(task2Buffer), task2, NULL,
                  OS_TaskPriorityLevel3_App, "T2");

    OS_Start ();

    Board_LED_Toggle (LEDS_LED1);

	return 0;
}
