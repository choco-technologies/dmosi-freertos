#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "dmod_sal.h"

/* Provide implementations of FreeRTOS heap functions
 * These may be called by FreeRTOS internals before our macros take effect */

#undef pvPortMalloc
#undef vPortFree

void* pvPortMalloc(size_t xWantedSize)
{
    const char* module_name = dmosi_thread_get_module_name(NULL);
    return Dmod_MallocEx(xWantedSize, module_name ? module_name : "unknown");
}

void vPortFree(void* pv)
{
    Dmod_Free(pv);
}

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
