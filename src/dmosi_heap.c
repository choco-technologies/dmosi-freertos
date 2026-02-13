#include "FreeRTOS.h"
#include "dmod_sal.h"
#include "dmosi.h"

/**
 * @brief Custom memory allocation function for FreeRTOS
 * 
 * This function is used by FreeRTOS for dynamic memory allocation. It redirects
 * to the DMOD memory allocator, passing the current thread's module name for
 * tracking purposes.
 * 
 * @param size Size of memory to allocate in bytes
 * @return void* Pointer to allocated memory, or NULL on failure
 */
void* pvPortMalloc(size_t size)
{
    return Dmod_MallocEx(size, dmosi_thread_get_module_name(NULL));
}

/**
 * @brief Custom memory deallocation function for FreeRTOS
 * 
 * This function is used by FreeRTOS for freeing dynamically allocated memory.
 * It redirects to the DMOD memory deallocator.
 * 
 * @param ptr Pointer to memory to free
 */
void vPortFree(void* ptr)
{
    Dmod_Free(ptr);
}

/**
 * @brief Get the current free heap size
 * 
 * This function is used by FreeRTOS to query the amount of free heap memory available.
 * Since we are using a custom allocator, we return 0 to indicate that this information
 * is not available.
 */
size_t xPortGetFreeHeapSize(void)
{
    return 0;
}

/**
 * @brief Get the minimum ever free heap size
 * 
 * This function is used by FreeRTOS to query the minimum amount of free heap memory
 * that has been available since the system started. Since we are using a custom allocator,
 * we return 0 to indicate that this information is not available.
 */
size_t xPortGetMinimumEverFreeHeapSize(void)
{
    return 0;
}

/**
 * @brief Initialize memory blocks (not needed for custom allocator)
 * 
 * This function is called by FreeRTOS to initialize memory blocks when using certain
 * heap implementations. Since we are using a custom allocator, we do not need to
 * perform any initialization here.
 */
void vPortInitialiseBlocks(void)
{
    /* Not needed */
}
