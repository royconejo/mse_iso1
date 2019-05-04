#include "board.h"

#include "retrociaa/base/systick.h"
#include "retrociaa/base/mempool.h"
#include "retrociaa/base/semaphore.h"

#include "retrociaa/os/api.h"
#include "retrociaa/os/mutex.h"

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
        const bool B1   = !Board_TEC_Pressed (BOARD_TEC_1);
        const bool B2   = !Board_TEC_Pressed (BOARD_TEC_2);
        const bool B1RF = getButtonAction (B1, &ad->measurements.b1);
        const bool B2RF = getButtonAction (B2, &ad->measurements.b2);

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
        // Fall/Rise de B1 sin Fall de B2 o viceversa resetea las mediciones
        else if ((B1RF && ad->measurements.b2.fall == OS_TicksUndefined) ||
                 (B2RF && ad->measurements.b1.fall == OS_TicksUndefined))
        {
            measurementsRestart (&ad->measurements);
            // Demora periodica de 40 milisegundos.
            r = OS_TaskPeriodicDelay (40);
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
    Board_Debug_Init        ();
	SystemCoreClockUpdate   ();
	SYSTICK_SetPeriod_ms    (1);

    uint8_t initBuffer [OS_InitBufferSize ()] ATTR_DataAlign8;

    struct AppData ad;

    OS_Init     (initBuffer);
    OS_Start    (boot, &ad);

    Board_LED_Toggle (LED_RED);

	return 0;
}
