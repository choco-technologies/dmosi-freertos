#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
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
    struct dmosi_thread* next;        /**< Next thread in the global list */
};

/**
 * @brief Global list of all active thread structures
 *
 * Protected by FreeRTOS critical sections.
 */
static struct dmosi_thread* thread_list_head = NULL;

/**
 * @brief Add a thread to the global thread list
 *
 * @param thread Thread structure to register
 */
static void thread_list_add(struct dmosi_thread* thread)
{
    taskENTER_CRITICAL();
    thread->next = thread_list_head;
    thread_list_head = thread;
    taskEXIT_CRITICAL();
}

/**
 * @brief Remove a thread from the global thread list
 *
 * @param thread Thread structure to deregister
 */
static void thread_list_remove(struct dmosi_thread* thread)
{
    taskENTER_CRITICAL();
    struct dmosi_thread** pp = &thread_list_head;
    while (*pp != NULL) {
        if (*pp == thread) {
            *pp = thread->next;
            thread->next = NULL;
            break;
        }
        pp = &(*pp)->next;
    }
    taskEXIT_CRITICAL();
}

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
                                       dmosi_process_t process)
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
    thread->next = NULL;
    
    return thread;
}

/**
 * @brief Wrapper function for FreeRTOS task entry
 * 
 * This function wraps the user's thread entry function to match
 * FreeRTOS task signature and handle cleanup.
 * 
 * @param pvParameters Pointer to dmod_thread structure
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

    struct dmosi_thread* thread = thread_new(NULL, entry, arg, process);
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

    thread_list_add(thread);
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
    
    // Clear the task-local storage pointer if this is the current thread
    // or if the task is still valid
    if (thread->handle != NULL) {
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
    
    thread_list_remove(thread);
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
        thread = thread_new(current_handle, NULL, NULL, dmosi_process_current());
        if (thread == NULL) {
            return NULL;
        }
        
        // Store in task-local storage for future calls
        vTaskSetThreadLocalStoragePointer(current_handle, DMOD_THREAD_TLS_INDEX, thread);
        thread_list_add(thread);
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
 * @brief Get an array of all threads
 *
 * Fills the provided array with handles of all existing threads.
 * If @p threads is NULL, returns the total number of threads.
 *
 * @param threads Pointer to array to fill, or NULL to query count only
 * @param max_count Maximum number of handles to write (ignored when @p threads is NULL)
 * @return size_t Number of threads (count query) or number of handles written
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, size_t, _thread_get_all, (dmosi_thread_t* threads, size_t max_count) )
{
    size_t count = 0;

    taskENTER_CRITICAL();
    struct dmosi_thread* t = thread_list_head;
    while (t != NULL) {
        if (threads != NULL) {
            if (count < max_count) {
                threads[count] = (dmosi_thread_t)t;
            }
        }
        count++;
        t = t->next;
    }
    taskEXIT_CRITICAL();

    // When writing to the array, return the number of handles actually written
    if (threads != NULL && count > max_count) {
        return max_count;
    }
    return count;
}

/**
 * @brief Get an array of threads belonging to a specific process
 *
 * Fills the provided array with handles of all threads associated with @p process.
 * If @p threads is NULL, returns the number of threads in that process.
 *
 * @param process Process handle whose threads to retrieve
 * @param threads Pointer to array to fill, or NULL to query count only
 * @param max_count Maximum number of handles to write (ignored when @p threads is NULL)
 * @return size_t Number of matching threads (count query) or number of handles written
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, size_t, _thread_get_by_process, (dmosi_process_t process, dmosi_thread_t* threads, size_t max_count) )
{
    size_t count = 0;

    taskENTER_CRITICAL();
    struct dmosi_thread* t = thread_list_head;
    while (t != NULL) {
        if (t->process == process) {
            if (threads != NULL) {
                if (count < max_count) {
                    threads[count] = (dmosi_thread_t)t;
                }
            }
            count++;
        }
        t = t->next;
    }
    taskEXIT_CRITICAL();

    // When writing to the array, return the number of handles actually written
    if (threads != NULL && count > max_count) {
        return max_count;
    }
    return count;
}
