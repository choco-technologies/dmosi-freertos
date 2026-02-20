#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "timers.h"

/**
 * @brief Internal structure to wrap FreeRTOS timer handle
 *
 * This structure wraps the FreeRTOS TimerHandle_t and stores
 * the user callback and argument so the FreeRTOS timer callback
 * can invoke the user-provided callback with the correct argument.
 */
struct dmosi_timer {
    TimerHandle_t handle;             /**< FreeRTOS timer handle */
    dmosi_timer_callback_t callback;  /**< User-provided callback */
    void* arg;                        /**< User-provided callback argument */
};

/**
 * @brief Convert milliseconds to FreeRTOS ticks, ensuring at least 1 tick
 *
 * @param ms Time in milliseconds
 * @return TickType_t Equivalent tick count (minimum 1 if ms > 0)
 */
static TickType_t ms_to_ticks(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ms > 0 && ticks == 0) {
        ticks = 1;
    }
    return ticks;
}

/**
 * @brief Internal FreeRTOS timer callback wrapper
 *
 * Translates the FreeRTOS timer callback signature to the dmosi
 * callback signature by retrieving the dmosi_timer structure from
 * the timer ID and invoking the user callback with the stored argument.
 *
 * @param xTimer FreeRTOS timer handle that expired
 */
static void timer_callback_wrapper(TimerHandle_t xTimer)
{
    struct dmosi_timer* timer = (struct dmosi_timer*)pvTimerGetTimerID(xTimer);

    if (timer != NULL && timer->callback != NULL) {
        timer->callback(timer->arg);
    }
}

//==============================================================================
//                              TIMER API Implementation
//==============================================================================

/**
 * @brief Create a timer
 *
 * Creates a FreeRTOS software timer with the specified period and
 * auto-reload behaviour.  The timer is created in the dormant state;
 * call dmosi_timer_start() to activate it.
 *
 * @param callback Callback function to execute when timer expires
 * @param arg Argument to pass to the callback function
 * @param period_ms Timer period in milliseconds
 * @param auto_reload Whether the timer should auto-reload
 * @return dmosi_timer_t Created timer handle, NULL on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, dmosi_timer_t, _timer_create, (dmosi_timer_callback_t callback, void* arg, uint32_t period_ms, bool auto_reload) )
{
    if (callback == NULL || period_ms == 0) {
        return NULL;
    }

    struct dmosi_timer* timer = (struct dmosi_timer*)pvPortMalloc(sizeof(*timer));
    if (timer == NULL) {
        return NULL;
    }

    timer->callback = callback;
    timer->arg = arg;

    TickType_t period_ticks = ms_to_ticks(period_ms);

    timer->handle = xTimerCreate(
        "dmosi_timer",
        period_ticks,
        auto_reload ? pdTRUE : pdFALSE,
        (void*)timer,
        timer_callback_wrapper
    );

    if (timer->handle == NULL) {
        vPortFree(timer);
        return NULL;
    }

    return (dmosi_timer_t)timer;
}

/**
 * @brief Destroy a timer
 *
 * Stops and destroys a timer, freeing associated resources.
 *
 * @param timer Timer handle to destroy
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, void, _timer_destroy, (dmosi_timer_t timer) )
{
    if (timer == NULL) {
        return;
    }

    if (timer->handle != NULL) {
        xTimerDelete(timer->handle, portMAX_DELAY);
    }

    vPortFree(timer);
}

/**
 * @brief Start a timer
 *
 * Starts a timer that is in the dormant state.
 *
 * @param timer Timer handle to start
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _timer_start, (dmosi_timer_t timer) )
{
    if (timer == NULL) {
        return -EINVAL;
    }

    BaseType_t result = xTimerStart(timer->handle, portMAX_DELAY);

    return (result == pdPASS) ? 0 : -EIO;
}

/**
 * @brief Stop a timer
 *
 * Stops an active timer.
 *
 * @param timer Timer handle to stop
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _timer_stop, (dmosi_timer_t timer) )
{
    if (timer == NULL) {
        return -EINVAL;
    }

    BaseType_t result = xTimerStop(timer->handle, portMAX_DELAY);

    return (result == pdPASS) ? 0 : -EIO;
}

/**
 * @brief Reset a timer
 *
 * Resets a timer.  If the timer is dormant, this starts it.
 * If it is already active, the expiry time is recalculated
 * relative to the current time.
 *
 * @param timer Timer handle to reset
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _timer_reset, (dmosi_timer_t timer) )
{
    if (timer == NULL) {
        return -EINVAL;
    }

    BaseType_t result = xTimerReset(timer->handle, portMAX_DELAY);

    return (result == pdPASS) ? 0 : -EIO;
}

/**
 * @brief Change timer period
 *
 * Changes the period of a timer.  If the timer is currently active,
 * the expiry time is updated accordingly.
 *
 * @param timer Timer handle
 * @param period_ms New timer period in milliseconds
 * @return int 0 on success, negative error code on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, int, _timer_set_period, (dmosi_timer_t timer, uint32_t period_ms) )
{
    if (timer == NULL || period_ms == 0) {
        return -EINVAL;
    }

    TickType_t period_ticks = ms_to_ticks(period_ms);

    BaseType_t result = xTimerChangePeriod(timer->handle, period_ticks, portMAX_DELAY);

    return (result == pdPASS) ? 0 : -EIO;
}

/**
 * @brief Get timer period
 *
 * Returns the period of the specified timer in milliseconds.
 *
 * @param timer Timer handle
 * @return uint32_t Timer period in milliseconds, 0 on failure
 */
DMOD_INPUT_API_DECLARATION( dmosi, 1.0, uint32_t, _timer_get_period, (dmosi_timer_t timer) )
{
    if (timer == NULL) {
        return 0;
    }

    TickType_t period_ticks = xTimerGetPeriod(timer->handle);

    return (uint32_t)(period_ticks * 1000 / configTICK_RATE_HZ);
}
