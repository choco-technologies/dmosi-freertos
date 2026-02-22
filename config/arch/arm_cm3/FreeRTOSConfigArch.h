/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M3 port.
 *
 * The ARM_CM3 port (portable/GCC/ARM_CM3) supports 16-bit and 32-bit tick
 * types only.
 *
 * No hardware FPU is present on Cortex-M3.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM3 port supports only 16-bit and 32-bit tick types. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
