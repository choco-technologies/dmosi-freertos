#include <stdbool.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"

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
