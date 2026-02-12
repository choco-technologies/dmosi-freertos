#include <stdlib.h>
#include <errno.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "semphr.h"

/**
 * @brief Internal structure to wrap FreeRTOS mutex handle
 * 
 * This structure wraps the FreeRTOS SemaphoreHandle_t and stores
 * whether the mutex is recursive, allowing us to use the correct
 * FreeRTOS API functions for lock/unlock operations.
 */
struct dmosi_mutex {
    SemaphoreHandle_t handle;  /**< FreeRTOS semaphore handle */
    bool recursive;            /**< Whether the mutex is recursive */
};

//==============================================================================
//                              MUTEX API Implementation
//==============================================================================

/**
 * @brief Create a mutex
 * 
 * Creates either a regular mutex or a recursive mutex based on the
 * recursive parameter using FreeRTOS API.
 * 
 * @param recursive Whether the mutex should be recursive
 * @return dmosi_mutex_t Created mutex handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_mutex_t, _mutex_create, (bool recursive) )
{
    struct dmosi_mutex* mutex = (struct dmosi_mutex*)pvPortMalloc(sizeof(*mutex));
    if (mutex == NULL) {
        return NULL;
    }

    if (recursive) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
    } else {
        mutex->handle = xSemaphoreCreateMutex();
    }

    if (mutex->handle == NULL) {
        vPortFree(mutex);
        return NULL;
    }

    mutex->recursive = recursive;
    return (dmosi_mutex_t)mutex;
}

/**
 * @brief Destroy a mutex
 * 
 * Destroys a mutex and frees associated resources.
 * 
 * @param mutex Mutex handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _mutex_destroy, (dmosi_mutex_t mutex) )
{
    if (mutex == NULL) {
        return;
    }

    struct dmosi_mutex* mtx = (struct dmosi_mutex*)mutex;
    
    if (mtx->handle != NULL) {
        vSemaphoreDelete(mtx->handle);
    }
    
    vPortFree(mtx);
}

/**
 * @brief Lock a mutex
 * 
 * Locks a mutex, blocking until the mutex is available.
 * Uses the appropriate FreeRTOS API based on whether the mutex is recursive.
 * 
 * @param mutex Mutex handle to lock
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _mutex_lock, (dmosi_mutex_t mutex) )
{
    if (mutex == NULL) {
        return -EINVAL;
    }

    struct dmosi_mutex* mtx = (struct dmosi_mutex*)mutex;
    BaseType_t result;

    if (mtx->recursive) {
        result = xSemaphoreTakeRecursive(mtx->handle, portMAX_DELAY);
    } else {
        result = xSemaphoreTake(mtx->handle, portMAX_DELAY);
    }

    return (result == pdTRUE) ? 0 : -EIO;
}

/**
 * @brief Unlock a mutex
 * 
 * Unlocks a previously locked mutex.
 * Uses the appropriate FreeRTOS API based on whether the mutex is recursive.
 * 
 * @param mutex Mutex handle to unlock
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _mutex_unlock, (dmosi_mutex_t mutex) )
{
    if (mutex == NULL) {
        return -EINVAL;
    }

    struct dmosi_mutex* mtx = (struct dmosi_mutex*)mutex;
    BaseType_t result;

    if (mtx->recursive) {
        result = xSemaphoreGiveRecursive(mtx->handle);
    } else {
        result = xSemaphoreGive(mtx->handle);
    }

    return (result == pdTRUE) ? 0 : -EPERM;
}
