#include <stdint.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "task.h"

//==============================================================================
//                              System Time API Implementation
//==============================================================================

/**
 * @brief Get the current system time in milliseconds
 *
 * Returns the number of milliseconds elapsed since the scheduler was started,
 * converting from FreeRTOS ticks using configTICK_RATE_HZ.
 *
 * @return uint32_t Elapsed time in milliseconds
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _get_tick_count, (void) )
{
    return (uint32_t)( (uint64_t)xTaskGetTickCount() * 1000ULL / configTICK_RATE_HZ );
}
