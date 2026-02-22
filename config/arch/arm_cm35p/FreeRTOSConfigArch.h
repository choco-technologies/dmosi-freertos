/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M35P
 * non-secure port.
 *
 * The ARM_CM35P_NONSECURE port (portable/GCC/ARM_CM35P/non_secure) supports
 * 16-bit and 32-bit tick types only.
 *
 * Cortex-M35P is an ARMv8-M Mainline processor with Physical Security
 * Extensions. It has the same capabilities as Cortex-M33 (FPU, TrustZone)
 * but no MVE.
 *
 * Note: configENABLE_TRUSTZONE and configRUN_FREERTOS_SECURE_ONLY must not
 * both be set to 1 simultaneously.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM35P port supports only 16-bit and 32-bit tick types. */
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

/* Cortex-M35P has a hardware FPU (FPv5-SP). */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif

/* Cortex-M35P has no MVE (M-Profile Vector Extension). */
#ifndef configENABLE_MVE
    #define configENABLE_MVE    0
#endif

/* Map FreeRTOS ARM Cortex-M35P interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the standard CMSIS exception names. */
#define SVC_Handler        dmosi_syscall_handler
#define PendSV_Handler     dmosi_context_switch_handler
#define SysTick_Handler    dmosi_tick_handler

#endif /* FREERTOS_CONFIG_ARCH_H */
