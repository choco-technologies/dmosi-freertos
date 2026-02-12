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
    
    if (thread != NULL && thread->entry != NULL) {
        thread->entry(thread->arg);
    }
    
    // Notify any task waiting to join
    if (thread != NULL && thread->joiner != NULL) {
        xTaskNotifyGive(thread->joiner);
    }
    
    // Task will be deleted when destroyed or will self-delete here
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
    thread->joined = false;
    thread->joiner = NULL;

    // FreeRTOS stack size is in words, not bytes
    // Convert bytes to words (assuming 32-bit words)
    UBaseType_t stack_words = stack_size / sizeof(StackType_t);
    
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
 * Note: In FreeRTOS, if a task is still running, it will be deleted forcefully.
 * 
 * @param thread Thread handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _thread_destroy, (dmod_thread_t thread) )
{
    if (thread == NULL) {
        return;
    }

    struct dmod_thread* thrd = (struct dmod_thread*)thread;
    
    // Delete the task if it still exists
    if (thrd->handle != NULL) {
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

    // Set the joiner to the current task
    thrd->joiner = xTaskGetCurrentTaskHandle();

    // Wait for notification from the thread when it completes
    // Use portMAX_DELAY for infinite wait
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    thrd->joined = true;
    return 0;
}

/**
 * @brief Get current thread
 * 
 * Returns a handle to the currently executing thread.
 * Note: This implementation creates a minimal thread structure for the
 * current task. The caller should not destroy this handle.
 * 
 * @return dmod_thread_t Current thread handle
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmod_thread_t, _thread_current, (void) )
{
    // For current thread, we return a simple wrapper
    // This is a limitation - we can't easily track all thread structures
    // In a production system, you might want a global registry
    static struct dmod_thread current_thread_cache;
    
    current_thread_cache.handle = xTaskGetCurrentTaskHandle();
    current_thread_cache.entry = NULL;
    current_thread_cache.arg = NULL;
    current_thread_cache.joined = false;
    current_thread_cache.joiner = NULL;
    
    if (current_thread_cache.handle == NULL) {
        return NULL;
    }
    
    return (dmod_thread_t)&current_thread_cache;
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
