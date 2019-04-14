#include "os_runtime.h"
#include "chip.h"   // cmsis.h


bool OS_RuntimeTask ()
{
    return (__get_CONTROL() & 0b10);
}


bool OS_RuntimePrivilegedTask ()
{
    return (__get_CONTROL() & 0b11) == 0b10;
}


bool OS_RuntimePrivileged ()
{
    return !(__get_CONTROL() & 0b10);
}
