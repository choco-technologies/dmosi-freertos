#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "dmosi.h"
#include "dmod.h"
#include "FreeRTOS.h"
#include "task.h"

static uint32_t g_state;
static bool g_seeded = false;

/**
 * @brief Advance and return the xorshift32 generator state
 *
 * Not cryptographically secure - see DMOSI_RAND_API group documentation in
 * dmosi.h.
 */
static uint32_t next_xorshift(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 17;
    g_state ^= g_state << 5;
    return g_state;
}

//==============================================================================
//                              Random Number API Implementation
//==============================================================================

/**
 * @brief Get a pseudo-random 32-bit value
 *
 * Not cryptographically secure - see DMOSI_RAND_API group documentation in
 * dmosi.h. Seeded on first use from the current tick count XORed with the
 * address of the generator state, since not every MCU family supported by
 * this backend has a hardware entropy source available. A real hardware
 * TRNG is out of scope here and belongs in its own driver/port module.
 *
 * portGET_RUN_TIME_COUNTER_VALUE() is deliberately not used for seeding:
 * it is only defined when configGENERATE_RUN_TIME_STATS is enabled and its
 * resolution/availability varies by port, so xTaskGetTickCount() is used
 * instead as it is guaranteed to be available on every port.
 *
 * @return uint32_t Pseudo-random value
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _rand32, (void) )
{
    taskENTER_CRITICAL();

    if (!g_seeded)
    {
        g_state = (uint32_t)xTaskGetTickCount() ^ (uint32_t)(uintptr_t)&g_state;

        // xorshift32 is degenerate at state 0 (it would stay 0 forever), so
        // nudge it to a fixed non-zero constant in that unlikely case.
        if (g_state == 0) g_state = 0x9E3779B9u;

        g_seeded = true;
    }

    uint32_t value = next_xorshift();

    taskEXIT_CRITICAL();

    return value;
}

/**
 * @brief Fill a buffer with pseudo-random bytes
 *
 * Not cryptographically secure - see DMOSI_RAND_API group documentation in
 * dmosi.h.
 *
 * @param buffer Output buffer, at least `len` bytes
 * @param len    Number of bytes to write
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _rand_bytes, (uint8_t* buffer, size_t len) )
{
    if (buffer == NULL) return;

    for (size_t i = 0; i < len; i += 4)
    {
        uint32_t value = dmosi_rand32();

        buffer[i] = (uint8_t)value;
        if (i + 1 < len) buffer[i + 1] = (uint8_t)(value >> 8);
        if (i + 2 < len) buffer[i + 2] = (uint8_t)(value >> 16);
        if (i + 3 < len) buffer[i + 3] = (uint8_t)(value >> 24);
    }
}
