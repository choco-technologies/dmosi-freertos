/*
 * Custom heap implementation for dmosi-freertos that redirects to DMOD allocator
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
    /* Not implemented - return 0 */
    return 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize(void)
{
    /* Not implemented - return 0 */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks(void)
{
    /* Not needed for DMOD allocator */
}
/*-----------------------------------------------------------*/
