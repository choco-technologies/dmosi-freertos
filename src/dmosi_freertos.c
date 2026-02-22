#include <stdbool.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"

//==============================================================================
//                              FreeRTOS Application Hooks
//==============================================================================

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )
/**
 * @brief Stack overflow hook function required by FreeRTOS when configCHECK_FOR_STACK_OVERFLOW is enabled
 * 
 * @param xTask Handle of the task that overflowed its stack
 * @param pcTaskName Name of the task that overflowed its stack
 */
__attribute__((weak)) void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    (void) xTask;
    DMOD_LOG_ERROR("Stack overflow detected in task: %s\n", pcTaskName);

    // Disable interrupts and halt the system
    taskDISABLE_INTERRUPTS();
    while(1);
}
#endif /* configCHECK_FOR_STACK_OVERFLOW */

extern void dmosi_thread_set_init_process(dmosi_process_t process);
extern void dmosi_thread_unregister_current(void);

static dmosi_process_t g_system_process = NULL;

//==============================================================================
//                              Initialization API Implementation
//==============================================================================

DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _init, (void) )
{
    if (g_system_process != NULL) {
        DMOD_LOG_ERROR("dmosi already initialized\n");
        return false;
    }

    g_system_process = dmosi_process_create("system", DMOSI_SYSTEM_MODULE_NAME, NULL);
    if (g_system_process == NULL) {
        DMOD_LOG_ERROR("Failed to create system process\n");
        return false;
    }

    dmosi_thread_set_init_process(g_system_process);

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        vTaskStartScheduler();
        dmosi_thread_set_init_process(NULL);
        return true;
    }

    if (dmosi_thread_current() == NULL) {
        DMOD_LOG_ERROR("Failed to bootstrap current thread\n");
        dmosi_thread_set_init_process(NULL);
        dmosi_process_destroy(g_system_process);
        g_system_process = NULL;
        return false;
    }

    dmosi_thread_set_init_process(NULL);
    return true;
}

DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _deinit, (void) )
{
    if (g_system_process == NULL) {
        return true;
    }

    dmosi_thread_set_init_process(NULL);
    dmosi_thread_unregister_current();
    dmosi_process_destroy(g_system_process);
    g_system_process = NULL;

    return true;
}
