#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "semphr.h"

/**
 * @brief Internal structure to wrap FreeRTOS semaphore handle
 * 
 * This structure wraps the FreeRTOS SemaphoreHandle_t.
 */
struct dmosi_semaphore {
    SemaphoreHandle_t handle;  /**< FreeRTOS semaphore handle */
};

//==============================================================================
//                              SEMAPHORE API Implementation
//==============================================================================

/**
 * @brief Create a semaphore
 * 
 * Creates a counting semaphore with the specified initial and maximum counts
 * using FreeRTOS API.
 * 
 * @param initial_count Initial count for the semaphore
 * @param max_count Maximum count for the semaphore
 * @return dmosi_semaphore_t Created semaphore handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_semaphore_t, _semaphore_create, (uint32_t initial_count, uint32_t max_count) )
{
    if (max_count == 0 || initial_count > max_count) {
        return NULL;
    }

    struct dmosi_semaphore* semaphore = (struct dmosi_semaphore*)pvPortMalloc(sizeof(*semaphore));
    if (semaphore == NULL) {
        return NULL;
    }

    semaphore->handle = xSemaphoreCreateCounting(max_count, initial_count);
    if (semaphore->handle == NULL) {
        vPortFree(semaphore);
        return NULL;
    }

    return (dmosi_semaphore_t)semaphore;
}

/**
 * @brief Destroy a semaphore
 * 
 * Destroys a semaphore and frees associated resources.
 * 
 * @param semaphore Semaphore handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _semaphore_destroy, (dmosi_semaphore_t semaphore) )
{
    if (semaphore == NULL) {
        return;
    }

    struct dmosi_semaphore* sem = (struct dmosi_semaphore*)semaphore;
    
    // Defensive check: handle should never be NULL for a valid semaphore,
    // but check anyway to prevent undefined behavior
    if (sem->handle != NULL) {
        vSemaphoreDelete(sem->handle);
    }
    
    vPortFree(sem);
}

/**
 * @brief Wait on a semaphore (decrement)
 * 
 * Waits on a semaphore, blocking until the semaphore count is greater than zero
 * or the timeout expires.
 * 
 * @param semaphore Semaphore handle
 * @param timeout_ms Timeout in milliseconds (0 = no wait, -1 = wait forever)
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _semaphore_wait, (dmosi_semaphore_t semaphore, int32_t timeout_ms) )
{
    if (semaphore == NULL) {
        return -EINVAL;
    }

    struct dmosi_semaphore* sem = (struct dmosi_semaphore*)semaphore;
    TickType_t ticks;
    
    if (timeout_ms < 0) {
        // Wait forever
        ticks = portMAX_DELAY;
    } else if (timeout_ms == 0) {
        // No wait
        ticks = 0;
    } else {
        // Convert milliseconds to ticks
        ticks = pdMS_TO_TICKS(timeout_ms);
    }

    BaseType_t result = xSemaphoreTake(sem->handle, ticks);
    
    if (result == pdTRUE) {
        return 0;
    } else if (ticks == 0) {
        return -EAGAIN;  // Would block
    } else {
        return -ETIMEDOUT;  // Timeout occurred
    }
}

/**
 * @brief Post to a semaphore (increment)
 * 
 * Increments the semaphore count, potentially unblocking a waiting thread.
 * 
 * @param semaphore Semaphore handle
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _semaphore_post, (dmosi_semaphore_t semaphore) )
{
    if (semaphore == NULL) {
        return -EINVAL;
    }

    struct dmosi_semaphore* sem = (struct dmosi_semaphore*)semaphore;
    BaseType_t result = xSemaphoreGive(sem->handle);
    
    return (result == pdTRUE) ? 0 : -EOVERFLOW;
}
