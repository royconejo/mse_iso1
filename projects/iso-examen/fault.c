#include "board.h"


void HardFault_Handler()
{
    while (1)
    {
        for (int i = 0; i < 10000000; ++i);
        Board_LED_Toggle (LED_RED);
    }
}


void MemManage_Handler ()
{
    while (1)
    {
        for (int i = 0; i < 10000000; ++i);
        Board_LED_Toggle (LED_GREEN);
    }
}


void BusFault_Handler ()
{
    while (1)
    {
        for (int i = 0; i < 10000000; ++i);
        Board_LED_Toggle (LED_BLUE);
    }
}

void UsageFault_Handler ()
{
    while (1)
    {
        for (int i = 0; i < 10000000; ++i);
        Board_LED_Toggle (LED_RED);
        Board_LED_Toggle (LED_BLUE);
        Board_LED_Toggle (LED_GREEN);
    }
}
