#include "appdata.h"
#include "board.h"
#include "retrociaa/base/variant.h"
#include "retrociaa/base/uart_util.h"


#define         MEMPOOL_SIZE            5120
#define         RECV_BUFFER_SIZE        128
#define         SEND_BUFFER_SIZE        512


uint8_t         g_mem[MEMPOOL_SIZE]     ATTR_DataAlign8;

const char *    g_TaskInputName         = "Input";
const char *    g_TaskLedName           = "Led";
const char *    g_TaskUartName          = "Uart";
const char *    g_LedNames[4]           = { "Rojo",
                                            "Verde",
                                            "Azul",
                                            "Amarillo" };
const char *    g_SendMessageText       =
                "Led %1 encendido:\n\r"
                "\t Tiempo encendido: %2 ms \n\r"
                "\t Tiempo entre flancos descendentes: %3 ms \n\r"
                "\t Tiempo entre flancos ascendentes: %4 ms ​\n\r";

const int       g_EdgeLedTable[2][2]    = { {LED_GREEN, LED_RED},
                                            {LED_1, LED_BLUE} };


bool getButtonAction (bool b, struct ButtonTimings *t)
{
    if (!b && t->fall == OS_TicksUndefined)
    {
        t->fall = OS_GetTicks ();
    }

    if (b && t->fall != OS_TicksUndefined && t->rise == OS_TicksUndefined)
    {
        t->rise = OS_GetTicks ();
    }

    return (t->fall != OS_TicksUndefined && t->rise != OS_TicksUndefined);
}


void getEdges (struct Measurements *m, struct LedParams *lp)
{
    const OS_Ticks B1F = m->b1.fall;
    const OS_Ticks B2F = m->b2.fall;
    const OS_Ticks B1R = m->b1.rise;
    const OS_Ticks B2R = m->b2.rise;

    lp->index       = g_EdgeLedTable [B1F > B2F][B1R > B2R];
    lp->fallTime    = (B1F > B2F) ? B1F - B2F : B2F - B1F;
    lp->riseTime    = (B1R > B2R) ? B1R - B2R : B2R - B1R;
}


void measurementsRestart (struct Measurements *m)
{
    m->b1.fall  = OS_TicksUndefined;
    m->b1.rise  = OS_TicksUndefined;
    m->b2.fall  = OS_TicksUndefined;
    m->b2.rise  = OS_TicksUndefined;

    m->started  = OS_GetTicks ();
    m->finished = OS_TicksUndefined;
}


void sendMessage (struct LedParams *lp)
{
    struct VARIANT args[4];

    VARIANT_SetString   (&args[0], g_LedNames[lp->index]);
    VARIANT_SetUint32   (&args[1], lp->fallTime + lp->riseTime);
    VARIANT_SetUint32   (&args[2], lp->fallTime);
    VARIANT_SetUint32   (&args[3], lp->riseTime);

    UART_PutMessageArgs (&lp->uart, g_SendMessageText, args, 4);
}


enum OS_Result appDataInit (struct AppData *ad)
{
    if (!MEMPOOL_Init (&ad->mempool, (uint32_t)g_mem, sizeof(g_mem)))
    {
        return OS_Result_NotInitialized;
    }

    ad->stacks.taskInput
        = MEMPOOL_Block (&ad->mempool,
                            OS_TaskMinBufferSize(OS_TaskType_Generic, NULL),
                            g_TaskLedName);

    ad->stacks.taskLed
        = MEMPOOL_Block (&ad->mempool,
                            OS_TaskMinBufferSize(OS_TaskType_Generic, NULL)
                            + 512,
                            g_TaskInputName);

    ad->stacks.taskUart
        = MEMPOOL_Block (&ad->mempool,
                            OS_TaskMinBufferSize(OS_TaskType_Generic, NULL),
                            g_TaskUartName);

    void * uartRecvBuffer
        = MEMPOOL_Block (&ad->mempool,
                            RECV_BUFFER_SIZE,
                            "UART-RECV");

    void * uartSendBuffer
        = MEMPOOL_Block (&ad->mempool,
                            SEND_BUFFER_SIZE,
                            "UART-SEND");

    if (!ad->stacks.taskInput
            || !ad->stacks.taskLed
            || !uartRecvBuffer
            || !uartSendBuffer)
    {
        return OS_Result_InvalidBuffer;
    }

    enum OS_Result r;
    if ((r = OS_MUTEX_Init (&ad->ledParams.mutex)) != OS_Result_OK)
    {
        return r;
    }

    if (!UART_Init (&ad->ledParams.uart, DEBUG_UART, 9600,
                    uartRecvBuffer, RECV_BUFFER_SIZE,
                    uartSendBuffer, SEND_BUFFER_SIZE))
    {
        return OS_Result_Error;
    }

    ad->ledParams.index     = 0;
    ad->ledParams.fallTime  = OS_TicksUndefined;
    ad->ledParams.riseTime  = OS_TicksUndefined;

    measurementsRestart (&ad->measurements);

    return OS_Result_OK;
}
