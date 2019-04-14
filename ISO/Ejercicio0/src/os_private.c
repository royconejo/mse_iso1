#include "os_private.h"
#include <string.h>


struct OS           *g_OS                       = NULL;
volatile uint32_t   g_OS_SchedulerTickBarrier   = 0;
volatile uint32_t   g_OS_SchedulerTicksMissed   = 0;
const char          *TaskNoDescription          = "N/A";
const char          *TaskBootDescription        = "BOOT";
const char          *TaskIdleDescription        = "IDLE";
