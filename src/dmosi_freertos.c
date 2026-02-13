#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"

/* Provide stub implementations for heap support functions */

size_t xPortGetFreeHeapSize(void)
{
    return 0;
}

size_t xPortGetMinimumEverFreeHeapSize(void)
{
    return 0;
}

void vPortInitialiseBlocks(void)
{
    /* Not needed */
}
