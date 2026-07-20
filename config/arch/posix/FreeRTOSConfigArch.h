/*
 * Architecture-specific FreeRTOS configuration for GCC POSIX (host/simulation) port.
 *
 * The POSIX port runs on a 64-bit host OS and supports 64-bit tick types.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* POSIX port supports 64-bit tick type for a wider tick range. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_64_BITS
#endif

/* The GCC/POSIX portmacro.h unconditionally defines portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
 * and portGET_RUN_TIME_COUNTER_VALUE after FreeRTOSConfig.h is processed.
 * Pre-define them here with the same values so that the #ifndef guards in
 * FreeRTOSConfig.h skip them and portmacro.h's redefinition is a no-op,
 * eliminating the macro-redefinition compiler warning. */
extern uint32_t ulPortGetRunTime( void );
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()    /* no-op */
#define portGET_RUN_TIME_COUNTER_VALUE()            ulPortGetRunTime()

/* The GCC/POSIX port does not implement xPortIsInsideInterrupt(), unlike the
 * hardware ports (Cortex-M, etc.). Application code on this simulated port
 * never truly runs in interrupt context: the only signal-based "interrupt"
 * (SIGALRM tick) is handled entirely inside the kernel port itself, so it is
 * always safe to report "not inside an interrupt" here. */
#define xPortIsInsideInterrupt()    ( 0 )

#endif /* FREERTOS_CONFIG_ARCH_H */
