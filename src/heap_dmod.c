/*
 * FreeRTOS heap implementation using DMOD allocator
 */

#include "FreeRTOS.h"
#include "task.h"
#include "dmod_sal.h"
#include "dmosi.h"

/*-----------------------------------------------------------*/

void* pvPortMalloc(size_t xWantedSize)
{
    const char* module_name = dmosi_thread_get_module_name(NULL);
    return Dmod_MallocEx(xWantedSize, module_name ? module_name : "unknown");
}
/*-----------------------------------------------------------*/

void vPortFree(void* pv)
{
    Dmod_Free(pv);
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize(void)
{
    return 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize(void)
{
    return 0;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks(void)
{
    /* Not needed */
}
/*-----------------------------------------------------------*/
