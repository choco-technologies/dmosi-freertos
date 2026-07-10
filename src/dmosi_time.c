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
 * converting from FreeRTOS ticks using configTICK_RATE_HZ. Safe to call from
 * both task and interrupt context: the FreeRTOS "FromISR" API is used
 * automatically when called from an interrupt handler.
 *
 * @return uint32_t Elapsed time in milliseconds
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _get_tick_count, (void) )
{
    TickType_t ticks = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

    return (uint32_t)( (uint64_t)ticks * 1000ULL / configTICK_RATE_HZ );
}
