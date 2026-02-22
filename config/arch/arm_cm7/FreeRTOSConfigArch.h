/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M7 (r0p1) port.
 *
 * The ARM_CM7/r0p1 port (portable/GCC/ARM_CM7/r0p1) supports ONLY 16-bit and
 * 32-bit tick types. Using TICK_TYPE_WIDTH_64_BITS will cause a compile error.
 *
 * The Cortex-M7 has a hardware FPU (FPv5-D16). The port requires the compiler
 * to be configured with hardware floating-point support. Ensure the toolchain
 * uses: -mfpu=fpv5-d16 -mfloat-abi=hard (or softfp).
 * This is configured automatically by the build system via
 * FREERTOS_ARCH_COMPILER_FLAGS when DMOSI_ARCH_FAMILY=cortex-m7 is set.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM7/r0p1 port supports ONLY 16-bit and 32-bit tick types. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
