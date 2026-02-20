#ifndef DMOSI_FREERTOS_INTERNAL_H
#define DMOSI_FREERTOS_INTERNAL_H

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "dmod.h"
#include "dmosi.h"

/**
 * @brief Task-local storage index for storing dmod_thread structure
 *
 * This index is used to store and retrieve the dmod_thread structure
 * associated with each FreeRTOS task.
 */
#define DMOD_THREAD_TLS_INDEX    0

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
    char module_name[DMOD_MAX_MODULE_NAME_LENGTH];  /**< Module name that created the thread */
    dmod_process_t process;        /**< Process that the thread belongs to */
};

/**
 * @brief Initialize the system process
 *
 * Creates and registers the FreeRTOS system process used as the default
 * process returned by dmosi_process_current() when no thread-specific
 * process has been set.  Must be called from dmosi_init().
 */
void dmosi_freertos_process_init(void);

/**
 * @brief Deinitialize the system process
 *
 * Destroys the system process created by dmosi_freertos_process_init().
 * Must be called from dmosi_deinit().
 */
void dmosi_freertos_process_deinit(void);

#endif /* DMOSI_FREERTOS_INTERNAL_H */
