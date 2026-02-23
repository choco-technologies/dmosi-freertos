#include <stdint.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "task.h"

//==============================================================================
//                              System Time API Implementation
//==============================================================================

/**
 * @brief Get the current RTOS tick count
 *
 * Returns the number of RTOS ticks elapsed since the scheduler was started.
 * The tick period depends on the RTOS configuration (typically 1 ms per tick).
 *
 * @return uint32_t Current tick count
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _get_tick_count, (void) )
{
    return (uint32_t)xTaskGetTickCount();
}
