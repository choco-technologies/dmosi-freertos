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

#endif /* FREERTOS_CONFIG_ARCH_H */
