/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-A9 port.
 *
 * The GCC_ARM_CA9 port supports 32-bit tick types.
 * Cortex-A9 is an ARMv7-A processor with hardware FPU.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
