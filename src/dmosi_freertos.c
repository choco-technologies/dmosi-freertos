#include <stdbool.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"

/* Forward declarations for internal helpers in dmosi_thread.c */
extern void dmosi_thread_set_init_process(dmosi_process_t process);
extern void dmosi_thread_unregister_current(void);

/**
 * @brief Handle of the system process created during initialization
 *
 * Tracks the process that owns the main task so that _deinit can clean it up.
 */
static dmosi_process_t g_system_process = NULL;

//==============================================================================
//                              Initialization API Implementation
//==============================================================================

/**
 * @brief Initialize the DMOSI FreeRTOS backend
 *
 * Creates the "system" process for the main task, sets it as the fallback
 * init process so that dmosi_thread_current() can bootstrap itself without
 * hitting infinite recursion through dmosi_process_current(), then forces
 * the lazy registration of the calling task's thread structure in TLS.
 *
 * @note Must be called exactly once, before vTaskStartScheduler() (i.e., in
 * a single-threaded context), and before any other DMOSI API.
 * DMOSI_SYSTEM_MODULE_NAME is defined in dmosi.h.
 *
 * @return bool true on success, false on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _init, (void) )
{
    if (g_system_process != NULL) {
        /* Already initialized */
        return true;
    }

    /* Create the root "system" process.
     * dmosi_process_create uses Dmod_MallocEx directly, so it does not go
     * through pvPortMalloc and avoids the circular dependency. */
    g_system_process = dmosi_process_create("system", DMOSI_SYSTEM_MODULE_NAME, NULL);
    if (g_system_process == NULL) {
        return false;
    }

    /* Set the fallback process so that the lazy-init path in _thread_current
     * uses g_system_process instead of recursively calling _process_current. */
    dmosi_thread_set_init_process(g_system_process);

    /* Bootstrap the current (main) task: _thread_current will allocate a
     * thread struct for the running task and store it in TLS, using
     * g_system_process as its owning process. */
    if (dmosi_thread_current() == NULL) {
        dmosi_thread_set_init_process(NULL);
        dmosi_process_destroy(g_system_process);
        g_system_process = NULL;
        return false;
    }

    /* Clear the fallback; from now on every new task created by
     * dmosi_thread_create has its own TLS entry set in thread_wrapper. */
    dmosi_thread_set_init_process(NULL);

    return true;
}

/**
 * @brief Deinitialize the DMOSI FreeRTOS backend
 *
 * Unregisters the current task's thread from TLS, then destroys the system
 * process created during _init.  After this call no DMOSI APIs should be
 * used until _init is called again.
 *
 * @return bool true on success, false on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _deinit, (void) )
{
    if (g_system_process == NULL) {
        /* Not initialized */
        return true;
    }

    /* Clear the init-process fallback in case _deinit is called while other
     * tasks are still starting up (defensive). */
    dmosi_thread_set_init_process(NULL);

    /* Remove and free the thread structure that was registered for the
     * calling (main) task during _init. */
    dmosi_thread_unregister_current();

    /* Destroy the system process (kills any remaining threads in it). */
    dmosi_process_destroy(g_system_process);
    g_system_process = NULL;

    return true;
}
