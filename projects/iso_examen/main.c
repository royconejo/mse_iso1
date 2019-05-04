#include "board.h"

#include "retrociaa/m4/base/systick.h"
#include "retrociaa/m4/base/mempool.h"
#include "retrociaa/m4/base/semaphore.h"

#include "retrociaa/m4/os/api.h"
#include "retrociaa/m4/os/mutex.h"

#include "appdata.h"


OS_TaskRetVal taskInput (OS_TaskParam arg)
{
    struct AppData  *ad = (struct AppData *) arg;
    enum OS_Result  r;

    (void) r;

    // Toma el mutex.
    r = OS_MUTEX_Lock (&ad->ledParams.mutex);

    // Setea inicio de la demora periodica.
    OS_TaskPeriodicDelay (0);
    while (1)
    {
        // Espera a que la tarea que maneja los leds termine de prenderlos.
        // Si esta es la 1er iteracion no hay nada que esperar ya que el mutex
        // fue tomado arriba.
        r = OS_TaskWaitForSignal (OS_TaskSignalType_MutexLock,
                                                     &ad->ledParams.mutex,
                                                     OS_WaitForever);
        // Actualiza estado de los botones.
        const uint32_t BTNS = Buttons_GetStatus ();
        const bool B1       = !(BTNS & TEC1_PRESSED);
        const bool B2       = !(BTNS & TEC2_PRESSED);
        const bool B1RF     = getButtonAction (B1, &ad->measurements.b1);
        const bool B2RF     = getButtonAction (B2, &ad->measurements.b2);

        // Se tomo el rise/fall de los dos botones?
        if (B1RF && B2RF)
        {
            ad->measurements.finished = OS_GetTicks ();
            // Obtiene tiempos y determina el led que corresponde prender.
            getEdges            (&ad->measurements, &ad->ledParams);
            measurementsRestart (&ad->measurements);
            // Desbloquea la tarea LED.
            r = OS_MUTEX_Unlock (&ad->ledParams.mutex);
            // Y cede el procesador.
            OS_TaskYield ();
        }
        else
        {
            // Demora periodica de 40 milisegundos.
            r = OS_TaskPeriodicDelay (40);
        }
    }

    return 0;
}


OS_TaskRetVal taskLed (OS_TaskParam arg)
{
    struct LedParams    *ledParams = (struct LedParams *) arg;
    enum OS_Result      r;

    (void) r;

    while (1)
    {
        // Espera a que la tarea de input determine un nuevo trabajo.
        r = OS_TaskWaitForSignal (OS_TaskSignalType_MutexLock,
                                                     &ledParams->mutex,
                                                     OS_WaitForever);
        // Prepara el mensaje a enviar por UART
        sendMessage     (ledParams);

        // Prende el led correspondiente la cantidad de tiempo indicada y
        // lo apaga pasada esa demora.
        Board_LED_Set   (ledParams->index, true);
        OS_TaskDelay    (ledParams->fallTime + ledParams->riseTime);
        Board_LED_Set   (ledParams->index, false);

        // Desbloquea la tarea de input y cede el procesador.
        OS_MUTEX_Unlock (&ledParams->mutex);
        OS_TaskYield    ();
    }

    return 0;
}


OS_TaskRetVal taskUart (OS_TaskParam arg)
{
    struct LedParams    *ledParams = (struct LedParams *) arg;
    enum OS_Result      r;

    (void) r;

    while (1)
    {
        while (UART_SendPendingCount (&ledParams->uart))
        {
            UART_Send (&ledParams->uart);
        }

        r = OS_TaskDelay (50);
    }

    return 0;
}


OS_TaskRetVal boot (OS_TaskParam arg)
{
    struct AppData  *ad = (struct AppData *) arg;
    enum OS_Result  r;

    if ((r = appDataInit(ad)) != OS_Result_OK)
    {
        return 1;
    }

    if ((r = OS_TaskStart (ad->stacks.taskInput,
                                  MEMPOOL_BlockSize(ad->stacks.taskInput),
                                  taskInput, ad,
                                  OS_TaskPriority_Kernel1,
                                  g_TaskInputName)) != OS_Result_OK)
    {
        return 2;
    }

    if ((r = OS_TaskStart (ad->stacks.taskLed,
                                  MEMPOOL_BlockSize(ad->stacks.taskLed),
                                  taskLed, &ad->ledParams,
                                  OS_TaskPriority_Kernel1,
                                  g_TaskLedName)) != OS_Result_OK)
    {
        return 3;
    }

    if ((r = OS_TaskStart (ad->stacks.taskUart,
                                  MEMPOOL_BlockSize(ad->stacks.taskUart),
                                  taskUart, &ad->ledParams,
                                  OS_TaskPriority_Kernel1,
                                  g_TaskUartName)) != OS_Result_OK)
    {
        return 4;
    }

    return 0;
}


int main ()
{
    Board_Init              ();
	SystemCoreClockUpdate   ();
	SYSTICK_SetPeriod_ms    (1);

    // LPC_USART2
    Chip_SCU_PinMuxSet      (7, 1, (SCU_MODE_INACT | SCU_MODE_FUNC6));
    Chip_SCU_PinMuxSet      (7, 2, (SCU_MODE_INACT | 
                                    SCU_MODE_INBUFF_EN | SCU_MODE_FUNC6));

    uint8_t initBuffer [OS_InitBufferSize ()] ATTR_DataAlign8;

    struct AppData ad;

    OS_Init     (initBuffer);
    OS_Start    (boot, &ad);

    Board_LED_Set (LED_RED, true);

	return 0;
}
