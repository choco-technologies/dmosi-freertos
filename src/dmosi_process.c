#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "dmosi_freertos_internal.h"

/**
 * @brief Internal structure representing a FreeRTOS-backed process
 *
 * Since FreeRTOS is a single-process RTOS, processes are emulated as
 * lightweight containers that group related threads and carry metadata
 * such as name, PID, UID, and working directory.
 */
struct dmod_process {
    char name[DMOD_MAX_MODULE_NAME_LENGTH];  /**< Process name */
    dmod_process_id_t pid;                   /**< Unique process ID */
    dmod_user_id_t uid;                      /**< User ID */
    char pwd[DMOD_MAX_PATH_LENGTH];          /**< Working directory */
    dmod_process_state_t state;              /**< Current process state */
    dmod_process_t parent;                   /**< Parent process, or NULL */
    int exit_status;                         /**< Exit status set by _process_kill */
    TaskHandle_t waiter;                     /**< Task waiting in _process_wait */
};

/** @brief System (root) process created during dmosi_init */
static struct dmod_process* g_system_process = NULL;

/** @brief Counter for assigning unique PIDs */
static dmod_process_id_t g_next_pid = 1;

/**
 * @brief Allocate and initialize a new process structure
 *
 * @param name  Process name (may be NULL)
 * @param parent Parent process handle (may be NULL)
 * @return Pointer to the new process, or NULL on allocation failure
 */
static struct dmod_process* process_new(const char* name, dmod_process_t parent)
{
    struct dmod_process* process = (struct dmod_process*)pvPortMalloc(sizeof(*process));
    if (process == NULL) {
        return NULL;
    }

    if (name != NULL) {
        strncpy(process->name, name, DMOD_MAX_MODULE_NAME_LENGTH - 1);
        process->name[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
    } else {
        process->name[0] = '\0';
    }

    taskENTER_CRITICAL();
    process->pid = g_next_pid++;
    taskEXIT_CRITICAL();

    process->uid    = 0;
    process->pwd[0] = '/';
    process->pwd[1] = '\0';
    process->state  = DMOSI_PROCESS_STATE_CREATED;
    process->parent = parent;
    process->exit_status = 0;
    process->waiter = NULL;

    return process;
}

//==============================================================================
//                     Internal helpers called by dmosi_freertos.c
//==============================================================================

/**
 * @brief Create and register the system (root) process
 *
 * Called from dmosi_init() during system startup.  Creates a process that
 * represents the ambient execution context before any dmosi threads are
 * spawned.  No-op if the system process already exists.
 */
void dmosi_freertos_process_init(void)
{
    if (g_system_process != NULL) {
        return;
    }

    g_system_process = process_new(DMOSI_SYSTEM_MODULE_NAME, NULL);
    if (g_system_process != NULL) {
        g_system_process->state = DMOSI_PROCESS_STATE_RUNNING;
    }
}

/**
 * @brief Destroy the system process created by dmosi_freertos_process_init()
 *
 * Called from dmosi_deinit().
 */
void dmosi_freertos_process_deinit(void)
{
    if (g_system_process != NULL) {
        vPortFree(g_system_process);
        g_system_process = NULL;
    }
}

//==============================================================================
//                              PROCESS API Implementation
//==============================================================================

/**
 * @brief Create a process
 *
 * Allocates a new process container.  The process starts in the
 * DMOSI_PROCESS_STATE_CREATED state; it becomes RUNNING once its first
 * thread begins execution.
 *
 * @param name   Process name (may be NULL)
 * @param parent Parent process handle (may be NULL)
 * @return dmod_process_t Created process handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_t, _process_create, (const char* name, dmod_process_t parent) )
{
    return (dmod_process_t)process_new(name, parent);
}

/**
 * @brief Destroy a process
 *
 * Frees the memory associated with the process handle.  The caller is
 * responsible for ensuring that no threads still reference this process.
 *
 * @param process Process handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _process_destroy, (dmod_process_t process) )
{
    if (process == NULL) {
        return;
    }

    vPortFree(process);
}

/**
 * @brief Kill a process
 *
 * Marks the process as terminated and notifies any task blocked in
 * dmosi_process_wait().
 *
 * @param process Process handle to kill
 * @param status  Exit status code
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_kill, (dmod_process_t process, int status) )
{
    if (process == NULL) {
        return -EINVAL;
    }

    TaskHandle_t waiter_to_notify = NULL;

    taskENTER_CRITICAL();
    process->exit_status = status;
    process->state       = DMOSI_PROCESS_STATE_TERMINATED;
    waiter_to_notify     = process->waiter;
    taskEXIT_CRITICAL();

    if (waiter_to_notify != NULL) {
        xTaskNotifyGive(waiter_to_notify);
    }

    return 0;
}

/**
 * @brief Wait for a process to terminate
 *
 * Blocks the calling task until the process reaches the TERMINATED (or
 * ZOMBIE) state, or until the timeout expires.
 *
 * @param process    Process handle to wait for
 * @param timeout_ms Timeout in milliseconds (0 = no wait, -1 = wait forever)
 * @return int 0 on success, -EAGAIN if timeout_ms == 0 and not terminated,
 *             -ETIMEDOUT on timeout, -EBUSY if another task is already waiting
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_wait, (dmod_process_t process, int32_t timeout_ms) )
{
    if (process == NULL) {
        return -EINVAL;
    }

    bool already_terminated = false;

    taskENTER_CRITICAL();
    already_terminated = (process->state == DMOSI_PROCESS_STATE_TERMINATED ||
                          process->state == DMOSI_PROCESS_STATE_ZOMBIE);

    if (!already_terminated) {
        if (process->waiter != NULL) {
            taskEXIT_CRITICAL();
            return -EBUSY;
        }
        process->waiter = xTaskGetCurrentTaskHandle();
    }
    taskEXIT_CRITICAL();

    if (already_terminated) {
        return 0;
    }

    TickType_t ticks = (timeout_ms < 0)  ? portMAX_DELAY :
                       (timeout_ms == 0) ? 0 :
                       pdMS_TO_TICKS(timeout_ms);

    BaseType_t result = (BaseType_t)ulTaskNotifyTake(pdTRUE, ticks);

    if (result == 0) {
        /* Timeout — clear the waiter registration */
        taskENTER_CRITICAL();
        process->waiter = NULL;
        taskEXIT_CRITICAL();
        return (timeout_ms == 0) ? -EAGAIN : -ETIMEDOUT;
    }

    return 0;
}

/**
 * @brief Get current process
 *
 * Returns the process associated with the currently executing FreeRTOS task.
 * If the task was created via dmosi_thread_create() the process stored in its
 * thread-local structure is returned; otherwise the system process is returned.
 *
 * @return dmod_process_t Current process handle
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_t, _process_current, (void) )
{
    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();

    if (current_handle != NULL) {
        struct dmod_thread* thread = (struct dmod_thread*)pvTaskGetThreadLocalStoragePointer(
            current_handle,
            DMOD_THREAD_TLS_INDEX
        );

        if (thread != NULL && thread->process != NULL) {
            return thread->process;
        }
    }

    return (dmod_process_t)g_system_process;
}

/**
 * @brief Set current process
 *
 * Updates the process associated with the currently executing task by writing
 * to the thread-local thread structure.
 *
 * @param process Process handle to set as current
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_set_current, (dmod_process_t process) )
{
    if (process == NULL) {
        return -EINVAL;
    }

    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();
    if (current_handle == NULL) {
        return -EINVAL;
    }

    struct dmod_thread* thread = (struct dmod_thread*)pvTaskGetThreadLocalStoragePointer(
        current_handle,
        DMOD_THREAD_TLS_INDEX
    );

    if (thread == NULL) {
        return -EINVAL;
    }

    thread->process = process;
    return 0;
}

/**
 * @brief Get process state
 *
 * @param process Process handle
 * @return dmod_process_state_t Current state, TERMINATED if handle is NULL
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_state_t, _process_get_state, (dmod_process_t process) )
{
    if (process == NULL) {
        return DMOSI_PROCESS_STATE_TERMINATED;
    }
    return process->state;
}

/**
 * @brief Get process ID
 *
 * @param process Process handle
 * @return dmod_process_id_t Process ID, 0 on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_id_t, _process_get_id, (dmod_process_t process) )
{
    if (process == NULL) {
        return 0;
    }
    return process->pid;
}

/**
 * @brief Set process ID
 *
 * @param process Process handle
 * @param pid     New process ID
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_set_id, (dmod_process_t process, dmod_process_id_t pid) )
{
    if (process == NULL) {
        return -EINVAL;
    }
    process->pid = pid;
    return 0;
}

/**
 * @brief Get process name
 *
 * @param process Process handle
 * @return const char* Process name, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, const char*, _process_get_name, (dmod_process_t process) )
{
    if (process == NULL) {
        return NULL;
    }
    return process->name;
}

/**
 * @brief Set process user ID
 *
 * @param process Process handle
 * @param uid     User ID to set
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_set_uid, (dmod_process_t process, dmod_user_id_t uid) )
{
    if (process == NULL) {
        return -EINVAL;
    }
    process->uid = uid;
    return 0;
}

/**
 * @brief Get process user ID
 *
 * @param process Process handle
 * @return dmod_user_id_t User ID, 0 on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_user_id_t, _process_get_uid, (dmod_process_t process) )
{
    if (process == NULL) {
        return 0;
    }
    return process->uid;
}

/**
 * @brief Set process working directory
 *
 * @param process Process handle
 * @param pwd     Working directory path
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _process_set_pwd, (dmod_process_t process, const char* pwd) )
{
    if (process == NULL || pwd == NULL) {
        return -EINVAL;
    }
    strncpy(process->pwd, pwd, DMOD_MAX_PATH_LENGTH - 1);
    process->pwd[DMOD_MAX_PATH_LENGTH - 1] = '\0';
    return 0;
}

/**
 * @brief Get process working directory
 *
 * @param process Process handle
 * @return const char* Working directory path, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, const char*, _process_get_pwd, (dmod_process_t process) )
{
    if (process == NULL) {
        return NULL;
    }
    return process->pwd;
}

/**
 * @brief Find a process by name
 *
 * In the current FreeRTOS implementation only the system process can be found
 * this way; dynamically-created processes are not tracked in a global registry.
 *
 * @param name Process name to search for
 * @return dmod_process_t Process handle, NULL if not found
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_t, _process_find_by_name, (const char* name) )
{
    if (name == NULL) {
        return NULL;
    }

    if (g_system_process != NULL &&
        strncmp(g_system_process->name, name, DMOD_MAX_MODULE_NAME_LENGTH) == 0) {
        return (dmod_process_t)g_system_process;
    }

    return NULL;
}

/**
 * @brief Find a process by process ID
 *
 * In the current FreeRTOS implementation only the system process can be found
 * this way; dynamically-created processes are not tracked in a global registry.
 *
 * @param pid Process ID to search for
 * @return dmod_process_t Process handle, NULL if not found
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_process_t, _process_find_by_id, (dmod_process_id_t pid) )
{
    if (g_system_process != NULL && g_system_process->pid == pid) {
        return (dmod_process_t)g_system_process;
    }

    return NULL;
}
