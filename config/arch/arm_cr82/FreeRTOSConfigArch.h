/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-R82 port.
 *
 * The GCC_ARM_CR82 port targets Cortex-R82 (ARMv8-R).
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
