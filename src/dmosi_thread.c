#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Internal structure to wrap FreeRTOS task handle
 * 
 * This structure wraps the FreeRTOS TaskHandle_t and stores thread-related
 * information needed for thread management operations.
 */
struct dmod_thread {
    TaskHandle_t handle;           /**< FreeRTOS task handle */
    dmod_thread_entry_t entry;     /**< Thread entry function */
    void* arg;                     /**< Argument passed to thread entry */
    bool completed;                /**< Whether thread has completed execution */
    bool joined;                   /**< Whether thread has been joined */
    TaskHandle_t joiner;           /**< Handle of task waiting to join */
};

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
    struct dmod_thread* thread = (struct dmod_thread*)pvParameters;
    TaskHandle_t joiner_to_notify = NULL;
    
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
 * @return dmod_thread_t Created thread handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_thread_t, _thread_create, (dmod_thread_entry_t entry, void* arg, int priority, size_t stack_size) )
{
    if (entry == NULL) {
        return NULL;
    }

    struct dmod_thread* thread = (struct dmod_thread*)pvPortMalloc(sizeof(*thread));
    if (thread == NULL) {
        return NULL;
    }

    thread->entry = entry;
    thread->arg = arg;
    thread->completed = false;
    thread->joined = false;
    thread->joiner = NULL;

    // FreeRTOS stack size is in words, not bytes
    // Convert bytes to words (rounding up to ensure sufficient stack)
    UBaseType_t stack_words = (stack_size + sizeof(StackType_t) - 1) / sizeof(StackType_t);
    
    BaseType_t result = xTaskCreate(
        thread_wrapper,
        "dmod_thread",
        stack_words,
        thread,
        priority,
        &thread->handle
    );

    if (result != pdPASS || thread->handle == NULL) {
        vPortFree(thread);
        return NULL;
    }

    return (dmod_thread_t)thread;
}

/**
 * @brief Destroy a thread
 * 
 * Destroys a thread and frees associated resources.
 * 
 * Special case: If this is called on a handle returned by dmosi_thread_current(),
 * it will only free the wrapper structure and NOT terminate the current thread.
 * 
 * If the thread is still running and is not the current thread, it will be
 * forcefully deleted.
 * 
 * @param thread Thread handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _thread_destroy, (dmod_thread_t thread) )
{
    if (thread == NULL) {
        return;
    }

    struct dmod_thread* thrd = (struct dmod_thread*)thread;
    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    
    // Only delete the task if:
    // 1. It hasn't completed yet (completed is false)
    // 2. It's not the current thread (to avoid self-deletion)
    if (!thrd->completed && thrd->handle != NULL && thrd->handle != current) {
        vTaskDelete(thrd->handle);
    }
    
    vPortFree(thrd);
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
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _thread_join, (dmod_thread_t thread) )
{
    if (thread == NULL) {
        return -EINVAL;
    }

    struct dmod_thread* thrd = (struct dmod_thread*)thread;
    bool already_completed = false;
    
    // Check if already joined or set joiner atomically
    taskENTER_CRITICAL();
    
    if (thrd->joined) {
        taskEXIT_CRITICAL();
        return -EINVAL;  // Already joined
    }
    
    if (thrd->joiner != NULL) {
        taskEXIT_CRITICAL();
        return -EBUSY;  // Another task is already joining
    }
    
    // Check if thread already completed
    already_completed = thrd->completed;
    
    if (!already_completed) {
        // Set the joiner to the current task
        thrd->joiner = xTaskGetCurrentTaskHandle();
    }
    
    taskEXIT_CRITICAL();
    
    // If thread hasn't completed yet, wait for notification
    if (!already_completed) {
        // Wait for notification from the thread when it completes
        // Use portMAX_DELAY for infinite wait
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    // Thread has completed, mark as joined
    taskENTER_CRITICAL();
    thrd->joined = true;
    taskEXIT_CRITICAL();
    
    return 0;
}

/**
 * @brief Get current thread
 * 
 * Returns a handle to the currently executing thread.
 * 
 * IMPORTANT: The returned handle is allocated dynamically and MUST be freed
 * by calling dmosi_thread_destroy() when no longer needed. However, note that
 * calling destroy on the current thread handle will NOT terminate the current
 * thread - it will only free the wrapper structure. This is a limitation of
 * the current design.
 * 
 * In a production system, you would maintain a global registry of thread
 * structures to return the actual thread structure for the current task,
 * avoiding this memory management issue.
 * 
 * @return dmod_thread_t Current thread handle, or NULL if allocation fails
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_thread_t, _thread_current, (void) )
{
    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();
    
    if (current_handle == NULL) {
        return NULL;
    }
    
    // Allocate a wrapper structure for the current thread
    // Caller must free this by calling dmosi_thread_destroy()
    struct dmod_thread* thread = (struct dmod_thread*)pvPortMalloc(sizeof(*thread));
    if (thread == NULL) {
        return NULL;
    }
    
    thread->handle = current_handle;
    thread->entry = NULL;
    thread->arg = NULL;
    thread->completed = false;
    thread->joined = false;
    thread->joiner = NULL;
    
    return (dmod_thread_t)thread;
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
    vTaskDelay(ticks);
}
