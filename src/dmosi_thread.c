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
    volatile bool completed;       /**< Whether thread has completed execution */
    volatile bool joined;          /**< Whether thread has been joined */
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
    
    if (thread != NULL && thread->entry != NULL) {
        thread->entry(thread->arg);
    }
    
    // Mark thread as completed before notifying
    if (thread != NULL) {
        thread->completed = true;
        
        // Notify any task waiting to join
        if (thread->joiner != NULL) {
            xTaskNotifyGive(thread->joiner);
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
 * Note: If the thread is still running, it will be forcefully deleted.
 * 
 * @param thread Thread handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _thread_destroy, (dmod_thread_t thread) )
{
    if (thread == NULL) {
        return;
    }

    struct dmod_thread* thrd = (struct dmod_thread*)thread;
    
    // Only delete the task if it hasn't completed yet
    // If completed is true, the task already self-deleted
    if (!thrd->completed && thrd->handle != NULL) {
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
    
    if (thrd->joined) {
        return -EINVAL;  // Already joined
    }
    
    if (thrd->joiner != NULL) {
        return -EBUSY;  // Another task is already joining
    }
    
    // If thread already completed, no need to wait
    if (thrd->completed) {
        thrd->joined = true;
        return 0;
    }

    // Set the joiner to the current task
    thrd->joiner = xTaskGetCurrentTaskHandle();

    // Wait for notification from the thread when it completes
    // Use portMAX_DELAY for infinite wait
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Thread has completed, mark as joined
    thrd->joined = true;
    return 0;
}

/**
 * @brief Get current thread
 * 
 * Returns a handle to the currently executing thread.
 * Note: This returns the raw FreeRTOS TaskHandle_t wrapped in a minimal
 * thread structure. The returned handle should NOT be destroyed or joined.
 * This is a limitation of the current implementation - ideally, we would
 * maintain a registry of all thread structures to return the actual one.
 * 
 * @return dmod_thread_t Current thread handle, or NULL if not in a task context
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_thread_t, _thread_current, (void) )
{
    TaskHandle_t current_handle = xTaskGetCurrentTaskHandle();
    
    if (current_handle == NULL) {
        return NULL;
    }
    
    // For the current thread, we allocate a minimal wrapper structure
    // The caller should NOT destroy this handle - it's for information only
    // In a production system, you would maintain a global registry to return
    // the actual thread structure
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
