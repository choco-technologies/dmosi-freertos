#include <stdint.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"

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

/**
 * @brief Report whether the caller is running in interrupt/exception context
 *
 * Overrides the dmod SAL default (which always answers false) so that the
 * stdio path can detect an ISR and keep Dmod_Printf/DMOD_LOG_* safe there -
 * see Dmod_IsInsideInterrupt() in dmod_sal.h for what that changes.
 *
 * xPortIsInsideInterrupt() is a plain IPSR read on ARMv7-M: no locking, no
 * logging, no RTOS state touched. That matters, because this runs inside the
 * logging path itself - anything heavier would recurse straight back into it.
 *
 * @return true if called from an interrupt/exception handler
 */
DMOD_INPUT_API_DECLARATION( Dmod, 1.0, bool, _IsInsideInterrupt, (void) )
{
    return xPortIsInsideInterrupt() != pdFALSE;
}
