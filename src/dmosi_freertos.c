#include <stdbool.h>
#include "dmosi_freertos_internal.h"

//==============================================================================
//                      Initialization API Implementation
//==============================================================================

/**
 * @brief Initialize the DMOD OSI system
 *
 * Creates the system (root) process that serves as the default process context
 * for tasks not created via dmosi_thread_create().  This function must be
 * called before any other DMOSI API is used.
 *
 * @return bool true on successful initialization, false on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _init, (void) )
{
    dmosi_freertos_process_init();
    return (dmosi_process_current() != NULL);
}

/**
 * @brief Deinitialize the DMOD OSI system
 *
 * Destroys the system process created by dmosi_init().  After calling this
 * function no other DMOSI API should be used until dmosi_init() is called again.
 *
 * @return bool true on successful deinitialization, false on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, bool, _deinit, (void) )
{
    dmosi_freertos_process_deinit();
    return true;
}
