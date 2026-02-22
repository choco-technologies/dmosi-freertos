/*
 * Architecture-specific FreeRTOS configuration for GCC RISC-V port.
 *
 * The GCC_RISC_V_GENERIC port targets RISC-V processors.
 * For RV32 (32-bit RISC-V) a 32-bit tick is used. For RV64 you may override
 * DMOSI_TICK_TYPE_WIDTH_IN_BITS to TICK_TYPE_WIDTH_64_BITS.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
