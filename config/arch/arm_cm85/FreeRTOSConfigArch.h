/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M85
 * non-secure port.
 *
 * The ARM_CM85_NONSECURE port (portable/GCC/ARM_CM85/non_secure) supports
 * 16-bit and 32-bit tick types only.
 *
 * Cortex-M85 is an ARMv8.1-M processor with MVE (M-Profile Vector Extension /
 * Helium), hardware FPU, and TrustZone. It is the highest-performance
 * Cortex-M processor.
 *
 * Note: configENABLE_MVE MUST be defined (0 or 1) for this port.
 * Note: configENABLE_TRUSTZONE and configRUN_FREERTOS_SECURE_ONLY must not
 * both be set to 1 simultaneously.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM85 port supports only 16-bit and 32-bit tick types. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* TrustZone disabled by default. */
#ifndef configENABLE_TRUSTZONE
    #define configENABLE_TRUSTZONE    0
#endif

/* Running on the non-secure side (or secure-only without TrustZone). */
#ifndef configRUN_FREERTOS_SECURE_ONLY
    #define configRUN_FREERTOS_SECURE_ONLY    0
#endif

/* MPU disabled by default. Set to 1 to enable the Memory Protection Unit. */
#ifndef configENABLE_MPU
    #define configENABLE_MPU    0
#endif

/* Cortex-M85 has a hardware FPU. */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif

/* Cortex-M85 has MVE (M-Profile Vector Extension). */
#ifndef configENABLE_MVE
    #define configENABLE_MVE    1
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
