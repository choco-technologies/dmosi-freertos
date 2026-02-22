/*
 * Architecture-specific FreeRTOS configuration for GCC ARM7TDMI / LPC2000 port.
 *
 * The GCC_ARM7_LPC2000 port targets ARM7TDMI (ARMv4T).
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
