/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-R5 port.
 *
 * The GCC_ARM_CR5 port targets Cortex-R5 (ARMv7-R).
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
