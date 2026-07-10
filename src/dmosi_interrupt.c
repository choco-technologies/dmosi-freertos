#include <stdint.h>
#include "dmosi.h"
#include "FreeRTOS.h"

//==============================================================================
//                              Interrupt Handler API Implementation
//==============================================================================

/**
 * @brief Get the minimum hardware interrupt priority allowed to call dmosi API
 *
 * Returns configMAX_SYSCALL_INTERRUPT_PRIORITY, the raw NVIC priority value
 * below which (i.e. numerically smaller / more urgent than) FreeRTOS ISR-safe
 * API functions must never be called, since such interrupts cannot be masked
 * by FreeRTOS critical sections.
 *
 * @return uint32_t Minimum (numerically) raw hardware priority value safe
 *         for calling dmosi API from interrupt context
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _get_min_interrupt_priority, (void) )
{
    return (uint32_t)configMAX_SYSCALL_INTERRUPT_PRIORITY;
}
