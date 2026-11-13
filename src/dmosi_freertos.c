#include <stdbool.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dmosi_freertos_internal.h"

//==============================================================================
//                              FreeRTOS Application Hooks
//==============================================================================

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )
/**
 * @brief Stack overflow hook function required by FreeRTOS when configCHECK_FOR_STACK_OVERFLOW is enabled
 *
 * A thread of a process only takes its own process down (see
 * dmosi_thread_stack_overflowed()). Anything else - a kernel task, a thread
 * of the system process - leaves nothing to recover: the system halts.
 *
 * @param xTask Handle of the task that overflowed its stack
 * @param pcTaskName Name of the task that overflowed its stack
 */
__attribute__((weak)) void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    if (dmosi_thread_stack_overflowed(xTask)) {
        return;
    }

    // This fires with an already overflowed stack, which may have clobbered adjacent
    // memory (e.g. this thread's dmosi_process linkage) that the normal Dmod_Printf
    // path depends on (Dmod_LockStdio -> dmosi_process_current() -> ...), and reading
    // corrupted state there can hard-fault a second time. Force raw kernel writes
    // first so DMOD_LOG_ERROR below stays safe without changing how it's written.
    Dmod_SetForceKernelWrite(true);
    DMOD_LOG_ERROR("Stack overflow detected in task: %s\n", pcTaskName);

    // Disable interrupts and halt the system
    taskDISABLE_INTERRUPTS();
    while(1);
}
#endif /* configCHECK_FOR_STACK_OVERFLOW */

extern void dmosi_thread_set_init_process(dmosi_process_t process);
extern void dmosi_thread_unregister_current(void);

static dmosi_process_t g_system_process = NULL;

bool dmosi_freertos_is_system_process(dmosi_process_t process)
{
    return process != NULL && process == g_system_process;
}

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

    if (!dmosi_thread_fault_supervisor_start(g_system_process)) {
        DMOD_LOG_ERROR("Failed to start the fault supervisor\n");
        dmosi_thread_set_init_process(NULL);
        dmosi_process_destroy(g_system_process);
        g_system_process = NULL;
        return false;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        vTaskStartScheduler();
        dmosi_thread_set_init_process(NULL);
        return true;
    }

    if (dmosi_thread_current() == NULL) {
        DMOD_LOG_ERROR("Failed to bootstrap current thread\n");
        dmosi_thread_set_init_process(NULL);
        dmosi_thread_fault_supervisor_stop();
        dmosi_process_destroy(g_system_process);
        g_system_process = NULL;
        return false;
    }

    dmosi_thread_set_init_process(NULL);
    return true;
}

DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _is_started, (void) )
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _deinit, (void) )
{
    if (g_system_process == NULL) {
        return true;
    }

    dmosi_thread_set_init_process(NULL);
    dmosi_thread_unregister_current();
    dmosi_thread_fault_supervisor_stop();
    dmosi_process_destroy(g_system_process);
    g_system_process = NULL;

    return true;
}
