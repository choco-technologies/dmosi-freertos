#include "dmosi.h"
#include "dmod_sal.h"

/**
 * @brief FreeRTOS memory allocation wrapper
 * 
 * This function wraps Dmod_MallocEx to track which module is allocating memory.
 * It gets the module name from the current thread and passes it to Dmod_MallocEx.
 * 
 * @param size Size of memory to allocate
 * @return void* Pointer to allocated memory, NULL on failure
 */
void* dmosi_port_malloc(size_t size)
{
    const char* module_name = dmosi_thread_get_module_name(NULL);
    return Dmod_MallocEx(size, module_name);
}

/**
 * @brief FreeRTOS memory free wrapper
 * 
 * This function wraps Dmod_Free for consistency.
 * 
 * @param ptr Pointer to memory to free
 */
void dmosi_port_free(void* ptr)
{
    Dmod_Free(ptr);
}
