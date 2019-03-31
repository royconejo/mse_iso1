#include "os.h"
#include "systick.h"
#include "board.h"


uint8_t     task1Buffer [OS_TaskMinBufferSize];
uint8_t     task2Buffer [OS_TaskMinBufferSize];


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
	while (1)
    {
        Board_LED_Toggle (LEDS_LED3);
        OS_TaskPeriodicDelay (500);
	}

	return 0;
}


OS_TaskRet task2 (OS_TaskParam arg)
{
	while (1)
    {
        Board_LED_Toggle (LEDS_LED2);
        OS_TaskPeriodicDelay (1000);
	}

	return 0;
}


int main ()
{
    Board_Init ();
	SystemCoreClockUpdate ();
	SYSTICK_SetMillisecondPeriod (1);

    struct OS os;

    OS_Init (&os);

    OS_TaskStart (task1Buffer, sizeof(task1Buffer), task1, NULL, OS_TaskPriorityLevel1);
    OS_TaskStart (task2Buffer, sizeof(task2Buffer), task2, NULL, OS_TaskPriorityLevel1);

    OS_Start ();

	return 0;
}
