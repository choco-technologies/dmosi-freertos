/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M33
 * non-secure port.
 *
 * The ARM_CM33_NONSECURE port (portable/GCC/ARM_CM33/non_secure) supports
 * 16-bit and 32-bit tick types only.
 *
 * Cortex-M33 is an ARMv8-M Mainline processor:
 *  - Has TrustZone (optional) and a hardware FPU (FPv5-SP).
 *  - configENABLE_MVE must be 0 or undefined (hardware error if set to 1).
 *
 * Note: configENABLE_TRUSTZONE and configRUN_FREERTOS_SECURE_ONLY must not
 * both be set to 1 simultaneously. The defaults here run FreeRTOS on the
 * non-secure side without TrustZone enabled. Set configENABLE_TRUSTZONE=1
 * and configRUN_FREERTOS_SECURE_ONLY=0 to enable TrustZone support.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM33 port supports only 16-bit and 32-bit tick types. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* TrustZone disabled by default. Set to 1 together with
 * configRUN_FREERTOS_SECURE_ONLY=0 to enable TrustZone support. */
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

/* Cortex-M33 has a hardware FPU (FPv5-SP). */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif

/* Cortex-M33 has no MVE (M-Profile Vector Extension). */
#ifndef configENABLE_MVE
    #define configENABLE_MVE    0
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
