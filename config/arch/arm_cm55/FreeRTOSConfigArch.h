/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M55
 * non-secure port.
 *
 * The ARM_CM55_NONSECURE port (portable/GCC/ARM_CM55/non_secure) supports
 * 16-bit and 32-bit tick types only.
 *
 * Cortex-M55 is an ARMv8.1-M processor with MVE (M-Profile Vector Extension /
 * Helium), hardware FPU, and TrustZone.
 *
 * Note: configENABLE_MVE MUST be defined (0 or 1) for this port.
 * Note: configENABLE_TRUSTZONE and configRUN_FREERTOS_SECURE_ONLY must not
 * both be set to 1 simultaneously.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM55 port supports only 16-bit and 32-bit tick types. */
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

/* Cortex-M55 has a hardware FPU. */
#ifndef configENABLE_FPU
    #define configENABLE_FPU    1
#endif

/* Cortex-M55 has MVE (M-Profile Vector Extension). */
#ifndef configENABLE_MVE
    #define configENABLE_MVE    1
#endif

/* Map FreeRTOS ARM Cortex-M55 interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the standard CMSIS exception names. */
#define SVC_Handler        dmosi_syscall_handler
#define PendSV_Handler     dmosi_context_switch_handler
#define SysTick_Handler    dmosi_tick_handler

/* Interrupt priority configuration for ARM Cortex-M55.
 *
 * On ARM Cortex-M Mainline (ARMv8.1-M), interrupt priorities are stored in
 * the most-significant bits of the 8-bit NVIC priority register. Priority 0
 * is the highest and is NOT maskable via BASEPRI, so
 * configMAX_API_CALL_INTERRUPT_PRIORITY must be non-zero.  The ARM_CM55 port
 * asserts this at scheduler start-up.
 *
 * The values below assume 4 NVIC priority bits (16 levels):
 *   - configKERNEL_INTERRUPT_PRIORITY         = 15 << (8-4) = 0xF0  (lowest priority)
 *   - configMAX_API_CALL_INTERRUPT_PRIORITY   =  5 << (8-4) = 0x50  (priority 5)
 *
 * If your device implements a different number of priority bits, adjust these
 * values using: priority_level << (8 - __NVIC_PRIO_BITS).
 * See https://www.freertos.org/RTOS-Cortex-M3-M4.html for details. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
    #define configKERNEL_INTERRUPT_PRIORITY    ( 15 << ( 8 - 4 ) )
#endif

#ifndef configMAX_SYSCALL_INTERRUPT_PRIORITY
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 << ( 8 - 4 ) )
#endif

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
    #define configMAX_API_CALL_INTERRUPT_PRIORITY    configMAX_SYSCALL_INTERRUPT_PRIORITY
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
