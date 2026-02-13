#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "queue.h"

/**
 * @brief Internal structure to wrap FreeRTOS queue handle
 * 
 * This structure wraps the FreeRTOS QueueHandle_t.
 */
struct dmosi_queue {
    QueueHandle_t handle;  /**< FreeRTOS queue handle */
};

//==============================================================================
//                              QUEUE API Implementation
//==============================================================================

/**
 * @brief Create a queue
 * 
 * Creates a queue with the specified item size and maximum length
 * using FreeRTOS API.
 * 
 * @param item_size Size of each item in the queue
 * @param queue_length Maximum number of items in the queue
 * @return dmosi_queue_t Created queue handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_queue_t, _queue_create, (size_t item_size, uint32_t queue_length) )
{
    if (item_size == 0 || queue_length == 0) {
        DMOD_LOG_ERROR("Invalid queue parameters: item_size=%zu, queue_length=%u\n", item_size, queue_length);
        return NULL;
    }

    struct dmosi_queue* queue = pvPortMalloc(sizeof(*queue));
    if (queue == NULL) {
        DMOD_LOG_ERROR("Failed to allocate memory for queue\n");
        return NULL;
    }

    queue->handle = xQueueCreate(queue_length, item_size);
    if (queue->handle == NULL) {
        DMOD_LOG_ERROR("Failed to create FreeRTOS queue\n");
        vPortFree(queue);
        return NULL;
    }

    return queue;
}

/**
 * @brief Destroy a queue
 * 
 * Destroys a queue and frees associated resources.
 * 
 * @param queue Queue handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _queue_destroy, (dmosi_queue_t queue) )
{
    if (queue == NULL) {
        return;
    }
    
    // Defensive check: handle should never be NULL for a valid queue,
    // but check anyway to prevent undefined behavior
    if (queue->handle != NULL) {
        vQueueDelete(queue->handle);
    }
    
    vPortFree(queue);
}

/**
 * @brief Send data to a queue
 * 
 * Sends an item to the back of the queue, blocking until space is available
 * or the timeout expires.
 * 
 * @param queue Queue handle
 * @param item Pointer to the item to send
 * @param timeout_ms Timeout in milliseconds (0 = no wait, -1 = wait forever)
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _queue_send, (dmosi_queue_t queue, const void* item, int32_t timeout_ms) )
{
    if (queue == NULL || item == NULL) {
        DMOD_LOG_ERROR("Invalid queue or item (NULL)\n");
        return -EINVAL;
    }

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

    BaseType_t result = xQueueSend(queue->handle, item, ticks);
    
    if (result == pdTRUE) {
        return 0;
    } else if (ticks == 0) {
        return -EAGAIN;  // Would block
    } else {
        return -ETIMEDOUT;  // Timeout occurred
    }
}

/**
 * @brief Receive data from a queue
 * 
 * Receives an item from the front of the queue, blocking until an item is
 * available or the timeout expires.
 * 
 * @param queue Queue handle
 * @param item Pointer to buffer to receive the item
 * @param timeout_ms Timeout in milliseconds (0 = no wait, -1 = wait forever)
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _queue_receive, (dmosi_queue_t queue, void* item, int32_t timeout_ms) )
{
    if (queue == NULL || item == NULL) {
        DMOD_LOG_ERROR("Invalid queue or item buffer (NULL)\n");
        return -EINVAL;
    }

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

    BaseType_t result = xQueueReceive(queue->handle, item, ticks);
    
    if (result == pdTRUE) {
        return 0;
    } else if (ticks == 0) {
        return -EAGAIN;  // Would block
    } else {
        return -ETIMEDOUT;  // Timeout occurred
    }
}
