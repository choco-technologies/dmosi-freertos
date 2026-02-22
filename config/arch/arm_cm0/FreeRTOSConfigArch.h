/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M0/M0+ port.
 *
 * The ARM_CM0 port (portable/GCC/ARM_CM0) supports 16-bit and 32-bit tick
 * types only. It also requires configENABLE_MPU to be explicitly defined.
 *
 * No hardware FPU is present on Cortex-M0/M0+.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* ARM_CM0 port supports only 16-bit and 32-bit tick types. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* Cortex-M0/M0+ has an optional MPU. Disabled by default; set to 1 to enable. */
#ifndef configENABLE_MPU
    #define configENABLE_MPU    0
#endif

/* Map FreeRTOS ARM Cortex-M0 interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the standard CMSIS exception names. */
#define SVC_Handler        dmosi_syscall_handler
#define PendSV_Handler     dmosi_context_switch_handler
#define SysTick_Handler    dmosi_tick_handler

#endif /* FREERTOS_CONFIG_ARCH_H */
