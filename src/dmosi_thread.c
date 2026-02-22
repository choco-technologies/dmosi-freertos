#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Task-local storage index for storing dmosi_thread structure
 * 
 * This index is used to store and retrieve the dmosi_thread structure
 * associated with each FreeRTOS task.
 */
#define DMOD_THREAD_TLS_INDEX    0

/**
 * @brief Fallback process used during system initialization
 *
 * Set by dmosi_thread_set_init_process() before the first call to
 * dmosi_thread_current(), so that the lazy-init path in _thread_current
 * does not need to call dmosi_process_current() and trigger infinite
 * recursion while the scheduler has not yet associated any process with
 * the running task.
 *
 * @note This variable is only written from dmosi_freertos _init/_deinit,
 * which must be called before the FreeRTOS scheduler is started (i.e., in
 * a single-threaded context). No synchronization is therefore required.
 */
static dmosi_process_t g_init_process = NULL;

/**
 * @brief Internal structure to wrap FreeRTOS task handle
 * 
 * This structure wraps the FreeRTOS TaskHandle_t and stores thread-related
 * information needed for thread management operations.
 */
struct dmosi_thread {
    TaskHandle_t handle;              /**< FreeRTOS task handle */
    dmosi_thread_entry_t entry;       /**< Thread entry function */
    void* arg;                        /**< Argument passed to thread entry */
    bool completed;                   /**< Whether thread has completed execution */
    bool joined;                      /**< Whether thread has been joined */
    TaskHandle_t joiner;              /**< Handle of task waiting to join */
    dmosi_process_t process;          /**< Process that the thread belongs to */
    size_t stack_size;                /**< Total stack size in bytes (0 if unknown) */
};

/**
 * @brief Helper function to create and initialize a new thread structure
 * 
 * Allocates memory for the thread structure and initializes it.
 * 
 * @param handle FreeRTOS task handle
 * @param entry Thread entry function (can be NULL)
 * @param arg Thread argument (can be NULL)
 * @param process Process to associate the thread with (can be NULL)
 * @return Pointer to initialized thread structure, NULL on allocation failure
 */
static struct dmosi_thread* thread_new(TaskHandle_t handle, 
                                       dmosi_thread_entry_t entry, 
                                       void* arg, 
                                       dmosi_process_t process,
                                       size_t stack_size)
{
    struct dmosi_thread* thread = (struct dmosi_thread*)pvPortMalloc(sizeof(*thread));
    if (thread == NULL) {
        return NULL;
    }
    
    thread->handle = handle;
    thread->entry = entry;
    thread->arg = arg;
    thread->completed = (entry == NULL);  // Mark as completed if no entry (e.g., main thread)
    thread->joined = false;
    thread->joiner = NULL;
    thread->process = process;
    thread->stack_size = stack_size;
    
    return thread;
}

/**
 * @brief Wrapper function for FreeRTOS task entry
 * 
 * This function wraps the user's thread entry function to match
 * FreeRTOS task signature and handle cleanup.
 * 
 * @param pvParameters Pointer to dmosi_thread structure
 */
static void thread_wrapper(void* pvParameters)
{
    struct dmosi_thread* thread = (struct dmosi_thread*)pvParameters;
    TaskHandle_t joiner_to_notify = NULL;
    
    // Store the thread structure in task-local storage so it can be
    // retrieved by dmosi_thread_current()
    if (thread != NULL) {
        vTaskSetThreadLocalStoragePointer(NULL, DMOD_THREAD_TLS_INDEX, thread);
    }
    
    if (thread != NULL && thread->entry != NULL) {
        thread->entry(thread->arg);
    }
    
    // Mark thread as completed and get joiner handle atomically
    if (thread != NULL) {
        taskENTER_CRITICAL();
        thread->completed = true;
        joiner_to_notify = thread->joiner;
        taskEXIT_CRITICAL();
        
        // Clear TLS before self-deletion so thread_enumerate won't return
        // a stale handle for this completed thread.
        vTaskSetThreadLocalStoragePointer(NULL, DMOD_THREAD_TLS_INDEX, NULL);
        
        // Notify any task waiting to join (outside critical section)
        if (joiner_to_notify != NULL) {
            xTaskNotifyGive(joiner_to_notify);
        }
    }
    
    // Task will self-delete here
    // Note: After this point, the task handle becomes invalid but the
    // thread structure remains valid until destroyed
    vTaskDelete(NULL);
}

//==============================================================================
//                              THREAD API Implementation
//==============================================================================

/**
 * @brief Create a thread
 * 
 * Creates a new thread (FreeRTOS task) with the specified entry function,
 * argument, priority, and stack size.
 * 
 * @param entry Entry function for the thread
 * @param arg Argument to pass to the entry function
 * @param priority Thread priority
 * @param stack_size Stack size for the thread in bytes
 * @param name Name of the thread (cannot be NULL)
 * @param process Process to associate the thread with (NULL = current process)
 * @return dmosi_thread_t Created thread handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_thread_t, _thread_create, (dmosi_thread_entry_t entry, void* arg, int priority, size_t stack_size, const char* name, dmosi_process_t process) )
{
    if (entry == NULL || stack_size == 0 || name == NULL) {
        return NULL;
    }

    // If no process provided, use the current process
    if (process == NULL) {
        process = dmosi_process_current();
    }

    struct dmosi_thread* thread = thread_new(NULL, entry, arg, process, stack_size);
    if (thread == NULL) {
        return NULL;
    }

    // FreeRTOS stack size is in words, not bytes
    // Convert bytes to words (rounding up to ensure sufficient stack)
    UBaseType_t stack_words = (stack_size + sizeof(StackType_t) - 1) / sizeof(StackType_t);
    
    BaseType_t result = xTaskCreate(
        thread_wrapper,
        name,
        stack_words,
        thread,
        priority,
        &thread->handle
    );

    if (result != pdPASS || thread->handle == NULL) {
        vPortFree(thread);
        return NULL;
    }

    return (dmosi_thread_t)thread;
}

/**
 * @brief Destroy a thread
 * 
 * Destroys a thread and frees associated resources.
 * 
 * This function clears the task-local storage pointer before freeing the
 * structure to ensure no dangling pointers remain.
 * 
 * If the thread is still running and is not the current thread, it will be
 * forcefully deleted.
 * 
 * @param thread Thread handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _thread_destroy, (dmosi_thread_t thread) )
{
    if (thread == NULL) {
        return;
    }

    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    
    // Only access TLS if the task has not completed (self-deleted).
    // After vTaskDelete(NULL) in thread_wrapper, the TCB may have been
    // freed by the idle task, making TLS access unsafe.
    if (thread->handle != NULL && !thread->completed) {
        // Check if the task-local storage still points to this structure
        void* stored = pvTaskGetThreadLocalStoragePointer(thread->handle, DMOD_THREAD_TLS_INDEX);
        if (stored == thread) {
            vTaskSetThreadLocalStoragePointer(thread->handle, DMOD_THREAD_TLS_INDEX, NULL);
        }
    }
    
    // Only delete the task if:
    // 1. It hasn't completed yet (completed is false)
    // 2. It's not the current thread (to avoid self-deletion)
    if (!thread->completed && thread->handle != NULL && thread->handle != current) {
        vTaskDelete(thread->handle);
    }
    
    vPortFree(thread);
}

/**
 * @brief Join a thread (wait for it to finish)
 * 
 * Waits for the specified thread to complete execution.
 * Uses FreeRTOS task notifications to be notified when the thread completes.
 * 
 * @param thread Thread handle to join
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _thread_join, (dmosi_thread_t thread) )
{
    if (thread == NULL) {
        return -EINVAL;
    }

    bool already_completed = false;
    
    // Check if already joined or set joiner atomically
    taskENTER_CRITICAL();
    
    if (thread->joined) {
        taskEXIT_CRITICAL();
        return -EINVAL;  // Already joined
    }
    
    if (thread->joiner != NULL) {
        taskEXIT_CRITICAL();
        return -EBUSY;  // Another task is already joining
    }
    
    // Check if thread already completed
    already_completed = thread->completed;
    
    if (!already_completed) {
        // Set the joiner to the current task
        thread->joiner = xTaskGetCurrentTaskHandle();
    }
    
    taskEXIT_CRITICAL();
    
    // If thread hasn't completed yet, wait for notification
    if (!already_completed) {
        // Wait for notification from the thread when it completes
        // Use portMAX_DELAY for infinite wait
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Double-check that the thread has actually completed
        // This guards against spurious notifications
        taskENTER_CRITICAL();
        while (!thread->completed) {
            taskEXIT_CRITICAL();
            // If not completed, wait again
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            taskENTER_CRITICAL();
        }
        taskEXIT_CRITICAL();
    }

    // Thread has completed, mark as joined
    taskENTER_CRITICAL();
    thread->joined = true;
    taskEXIT_CRITICAL();
    
    return 0;
}

/**
 * @brief Get current thread
 * 
 * Returns a handle to the currently executing thread.
 * 
 * This implementation uses FreeRTOS task-local storage to maintain a single
 * thread structure per task. The first time this is called for a task,
 * it allocates and stores the structure. Subsequent calls return the same
 * structure.
 * 
 * For tasks created with dmosi_thread_create, the structure is already stored
 * during creation. For other tasks (e.g., the main task or tasks created
 * directly with FreeRTOS), a structure is allocated on first call.
 * 
 * The returned handle should be passed to dmosi_thread_destroy() to free
 * the allocated structure when it's no longer needed.
 * 
 * @return dmosi_thread_t Current thread handle, or NULL if allocation fails
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_thread_t, _thread_current, (void) )
{
    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();
    
    if (current_handle == NULL) {
        return NULL;
    }
    
    // Try to retrieve existing thread structure from task-local storage
    struct dmosi_thread* thread = (struct dmosi_thread*)pvTaskGetThreadLocalStoragePointer(
        current_handle, 
        DMOD_THREAD_TLS_INDEX
    );
    
    // If no structure exists, allocate and store one
    if (thread == NULL) {
        // Use g_init_process as a fallback during system initialization to break
        // the circular dependency: _thread_current -> _process_current -> _thread_current.
        // Once init is complete, g_init_process is cleared and subsequent tasks
        // created via dmosi_thread_create already have TLS populated.
        dmosi_process_t process = (g_init_process != NULL) ? g_init_process : dmosi_process_current();
        thread = thread_new(current_handle, NULL, NULL, process, 0 /* stack_size unknown for externally created tasks */);
        if (thread == NULL) {
            return NULL;
        }
        
        // Store in task-local storage for future calls
        vTaskSetThreadLocalStoragePointer(current_handle, DMOD_THREAD_TLS_INDEX, thread);
    }
    
    return (dmosi_thread_t)thread;
}

/**
 * @brief Sleep for a specified time in milliseconds
 * 
 * Suspends the current thread for the specified number of milliseconds.
 * 
 * @param ms Time to sleep in milliseconds
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _thread_sleep, (uint32_t ms) )
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    
    // Ensure we delay at least 1 tick if ms > 0
    // vTaskDelay(0) just yields to equal priority tasks without blocking
    if (ms > 0 && ticks == 0) {
        ticks = 1;
    }
    
    vTaskDelay(ticks);
}

/**
 * @brief Get thread name
 * 
 * Returns the name of the specified thread, or the current thread if NULL.
 * 
 * @param thread Thread handle (if NULL, returns name of current thread)
 * @return const char* Thread name, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, const char*, _thread_get_name, (dmosi_thread_t thread) )
{
    // If thread is NULL, get current thread
    if (thread == NULL) {
        thread = dmosi_thread_current();
        if (thread == NULL) {
            return NULL;
        }
    }
    
    // Get name from FreeRTOS (FreeRTOS stores the name)
    return pcTaskGetName(thread->handle);
}

/**
 * @brief Get thread module name
 * 
 * Returns the module name associated with the thread by retrieving it from
 * the thread's associated process.
 * 
 * @param thread Thread handle (if NULL, returns module name of current thread)
 * @return const char* Module name of the process that owns the thread, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, const char*, _thread_get_module_name, (dmosi_thread_t thread) )
{
    // If thread is NULL, get current thread
    if (thread == NULL) {
        thread = dmosi_thread_current();
        if (thread == NULL) {
            return NULL;
        }
    }
    
    if (thread->process == NULL) {
        return NULL;
    }
    
    return dmosi_process_get_module_name(thread->process);
}

/**
 * @brief Get thread priority
 * 
 * Returns the priority of the specified thread, or the current thread if NULL.
 * 
 * @param thread Thread handle (if NULL, returns priority of current thread)
 * @return int Thread priority, or 0 on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _thread_get_priority, (dmosi_thread_t thread) )
{
    // If thread is NULL, get current thread
    if (thread == NULL) {
        thread = dmosi_thread_current();
        if (thread == NULL) {
            return 0;
        }
    }

    return (int)uxTaskPriorityGet(thread->handle);
}

/**
 * @brief Get thread's associated process
 * 
 * Returns the process handle that the specified thread belongs to, or the
 * current thread's process if NULL.
 * 
 * @param thread Thread handle (if NULL, returns process of current thread)
 * @return dmosi_process_t Process handle that the thread belongs to, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_process_t, _thread_get_process, (dmosi_thread_t thread) )
{
    // If thread is NULL, get current thread
    if (thread == NULL) {
        thread = dmosi_thread_current();
        if (thread == NULL) {
            return NULL;
        }
    }

    return thread->process;
}

/**
 * @brief Kill a thread
 *
 * Forcefully terminates a thread and marks it as completed.
 *
 * @param thread Thread handle to kill
 * @param status Exit status (not observable by other threads)
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _thread_kill, (dmosi_thread_t thread, int status) )
{
    if (thread == NULL) {
        return -EINVAL;
    }

    (void)status;

    TaskHandle_t current = xTaskGetCurrentTaskHandle();

    // Mark thread as completed and notify any joiner
    taskENTER_CRITICAL();
    thread->completed = true;
    TaskHandle_t joiner_to_notify = thread->joiner;
    taskEXIT_CRITICAL();

    if (joiner_to_notify != NULL) {
        xTaskNotifyGive(joiner_to_notify);
    }

    // Delete the FreeRTOS task if it is not the current task
    if (thread->handle != NULL && thread->handle != current) {
        vTaskDelete(thread->handle);
    } else if (thread->handle != NULL && thread->handle == current) {
        // Self-termination: delete this task; does not return
        vTaskDelete(NULL);
    }

    return 0;
}

/**
 * @brief Safety margin added to the task-array allocation to guard against
 *        tasks being created between uxTaskGetNumberOfTasks() and
 *        uxTaskGetSystemState().
 */
#define THREAD_ENUM_MARGIN    4

/**
 * @brief Enumerate all FreeRTOS tasks that have a dmosi_thread in TLS.
 *
 * Allocates a temporary TaskStatus_t array, calls uxTaskGetSystemState(), and
 * for each task whose TLS slot 0 is non-NULL, optionally writes the handle to
 * @p threads up to @p max_count entries.
 *
 * @param process  Filter: only include threads whose process matches this value.
 *                 Pass NULL to include threads regardless of process.
 * @param threads  Output array, or NULL for a count-only query.
 * @param max_count Maximum entries to write into @p threads.
 * @return Number of matching threads found, capped at @p max_count when @p threads
 *         is non-NULL.
 */
static size_t thread_enumerate(dmosi_process_t process, dmosi_thread_t* threads, size_t max_count)
{
    // Add a small margin to guard against new tasks being created between the
    // count query and the actual enumeration call.
    UBaseType_t alloc_count = uxTaskGetNumberOfTasks() + THREAD_ENUM_MARGIN;
    TaskStatus_t* task_array = (TaskStatus_t*)pvPortMalloc(alloc_count * sizeof(TaskStatus_t));
    if (task_array == NULL) {
        return 0;
    }

    UBaseType_t filled = uxTaskGetSystemState(task_array, alloc_count, NULL);
    size_t count = 0;

    for (UBaseType_t i = 0; i < filled; i++) {
        struct dmosi_thread* t = (struct dmosi_thread*)pvTaskGetThreadLocalStoragePointer(
            task_array[i].xHandle, DMOD_THREAD_TLS_INDEX);
        if (t == NULL) {
            continue;
        }
        if (process != NULL && t->process != process) {
            continue;
        }
        if (threads != NULL && count < max_count) {
            threads[count] = (dmosi_thread_t)t;
        }
        count++;
    }

    vPortFree(task_array);

    // When writing to the array, cap the return value at the number of handles written.
    if (threads != NULL && count > max_count) {
        return max_count;
    }
    return count;
}

/**
 * @brief Get an array of all threads
 *
 * Fills the provided array with handles of all existing threads by enumerating
 * FreeRTOS tasks and retrieving the dmosi_thread structure stored in TLS.
 * If @p threads is NULL, returns the total number of threads.
 *
 * @param threads Pointer to array to fill, or NULL to query count only
 * @param max_count Maximum number of handles to write (ignored when @p threads is NULL)
 * @return size_t Number of threads (count query) or number of handles written
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, size_t, _thread_get_all, (dmosi_thread_t* threads, size_t max_count) )
{
    return thread_enumerate(NULL, threads, max_count);
}

/**
 * @brief Get an array of threads belonging to a specific process
 *
 * Fills the provided array with handles of all threads associated with @p process
 * by enumerating FreeRTOS tasks and checking the dmosi_thread structure in TLS.
 * If @p threads is NULL, returns the number of threads in that process.
 *
 * @param process Process handle whose threads to retrieve
 * @param threads Pointer to array to fill, or NULL to query count only
 * @param max_count Maximum number of handles to write (ignored when @p threads is NULL)
 * @return size_t Number of matching threads (count query) or number of handles written
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, size_t, _thread_get_by_process, (dmosi_process_t process, dmosi_thread_t* threads, size_t max_count) )
{
    return thread_enumerate(process, threads, max_count);
}

/**
 * @brief Get information about a thread
 *
 * Fills the @p info structure with stack usage statistics, state, CPU usage,
 * and runtime for the given thread.  If @p thread is NULL, the current thread
 * is used.
 *
 * Stack peak/current values are derived from FreeRTOS's high-water mark
 * (minimum free stack ever observed).  CPU usage and runtime are reported as
 * zero because run-time statistics are not enabled in the default build
 * configuration.
 *
 * @param thread Thread handle (NULL = current thread)
 * @param info   Pointer to a dmosi_thread_info_t structure to fill
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _thread_get_info, (dmosi_thread_t thread, dmosi_thread_info_t* info) )
{
    if (info == NULL) {
        return -EINVAL;
    }

    if (thread == NULL) {
        thread = dmosi_thread_current();
        if (thread == NULL) {
            return -EFAULT;
        }
    }

    // If the task has already completed, return terminated state.
    // A thread is truly terminated when:
    //   - its FreeRTOS handle is NULL, OR
    //   - it was created via dmosi_thread_create (entry != NULL) and has finished
    // Lazy-init threads (entry == NULL) have completed=true but are still alive.
    if (thread->handle == NULL || (thread->entry != NULL && thread->completed)) {
        info->stack_total   = thread->stack_size;
        info->stack_current = 0;
        info->stack_peak    = 0;
        info->state         = DMOSI_THREAD_STATE_TERMINATED;
        info->cpu_usage     = 0.0f;
        info->runtime_ms    = 0;
        return 0;
    }

    TaskStatus_t task_status;
    vTaskGetInfo(thread->handle, &task_status, pdTRUE, eInvalid);

    // Map FreeRTOS task state to dmosi_thread_state_t
    dmosi_thread_state_t state;
    switch (task_status.eCurrentState) {
        case eRunning:   state = DMOSI_THREAD_STATE_RUNNING;    break;
        case eReady:     state = DMOSI_THREAD_STATE_READY;      break;
        case eBlocked:   state = DMOSI_THREAD_STATE_BLOCKED;    break;
        case eSuspended: state = DMOSI_THREAD_STATE_SUSPENDED;  break;
        case eDeleted:   state = DMOSI_THREAD_STATE_TERMINATED; break;
        default:         state = DMOSI_THREAD_STATE_TERMINATED; break;
    }

    // usStackHighWaterMark is the minimum free stack space in words (StackType_t units).
    // Peak usage = total stack - minimum free space ever observed.
    // FreeRTOS does not expose instantaneous stack usage, so stack_current is not available.
    size_t free_bytes = (size_t)task_status.usStackHighWaterMark * sizeof(StackType_t);
    size_t peak_usage = (thread->stack_size > free_bytes) ? (thread->stack_size - free_bytes) : 0;

    info->stack_total   = thread->stack_size;
    info->stack_current = 0;       // Not measurable by FreeRTOS at arbitrary call sites
    info->stack_peak    = peak_usage;
    info->state         = state;

    // Compute cpu_usage from the FreeRTOS run-time stats counter.
    // task_status.ulRunTimeCounter holds the accumulated counter ticks for this task.
    // portGET_RUN_TIME_COUNTER_VALUE() returns the current total elapsed counter value.
    // Clamp to [0, 100] because sampling the task counter and total counter at different
    // instants can make the ratio slightly exceed 100% due to scheduling jitter.
    configRUN_TIME_COUNTER_TYPE total_runtime = portGET_RUN_TIME_COUNTER_VALUE();
    if (total_runtime > 0U) {
        float usage = (float)task_status.ulRunTimeCounter / (float)total_runtime * 100.0f;
        info->cpu_usage = (usage < 100.0f) ? usage : 100.0f;
    } else {
        info->cpu_usage = 0.0f;
    }

    // Compute runtime_ms.  The counter unit depends on portGET_RUN_TIME_COUNTER_VALUE():
    // on POSIX it returns tms_utime ticks (CLK_TCK per second); convert via sysconf.
    // Divide before multiplying (seconds * 1000 + sub-second remainder * 1000 / clk_tck)
    // to avoid overflow when the counter value is large.
    // On other architectures, runtime_ms is left as 0 (counter frequency is unknown
    // without a port-specific conversion factor).
#if defined(__unix__) || defined(__APPLE__)
    {
        long clk_tck = sysconf(_SC_CLK_TCK);
        if (clk_tck > 0) {
            uint64_t counter = (uint64_t)task_status.ulRunTimeCounter;
            uint64_t freq    = (uint64_t)clk_tck;
            info->runtime_ms = (counter / freq) * 1000ULL
                               + (counter % freq) * 1000ULL / freq;
        } else {
            info->runtime_ms = 0;
        }
    }
#else
    info->runtime_ms = 0;
#endif

    return 0;
}

//==============================================================================
//                              Initialization helpers
//==============================================================================

/**
 * @brief Set the fallback process used during system initialization
 *
 * Must be called from dmosi_freertos _init before any other dmosi API that
 * could trigger dmosi_thread_current(), so that the lazy-init path in
 * _thread_current can associate the task with the correct process without
 * causing infinite recursion through dmosi_process_current().
 *
 * Pass NULL to clear the fallback after initialization is complete.
 *
 * @note This function is only called from _init/_deinit, which run before
 * the FreeRTOS scheduler starts (single-threaded context). No
 * synchronization is needed.
 *
 * @param process Process handle to use as fallback (NULL to clear)
 */
void dmosi_thread_set_init_process(dmosi_process_t process)
{
    g_init_process = process;
}

/**
 * @brief Unregister the current task's dmosi thread and clear TLS
 *
 * Clears the TLS entry for the running task and frees the associated
 * dmosi_thread structure.  Called from dmosi_freertos _deinit to clean
 * up the thread that was implicitly registered for the main task during
 * _init.
 */
void dmosi_thread_unregister_current(void)
{
    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();
    if (current_handle == NULL) {
        return;
    }

    struct dmosi_thread* thread = (struct dmosi_thread*)pvTaskGetThreadLocalStoragePointer(
        current_handle, DMOD_THREAD_TLS_INDEX);

    if (thread != NULL) {
        vTaskSetThreadLocalStoragePointer(current_handle, DMOD_THREAD_TLS_INDEX, NULL);
        vPortFree(thread);
    }
}
