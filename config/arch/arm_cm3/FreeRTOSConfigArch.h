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

/* Map FreeRTOS ARM Cortex-M3 interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the FreeRTOS-internal names. */
#define vPortSVCHandler        dmosi_syscall_handler
#define xPortPendSVHandler     dmosi_context_switch_handler
#define xPortSysTickHandler    dmosi_tick_handler

/* Interrupt priority configuration for ARM Cortex-M3.
 *
 * On ARM Cortex-M, interrupt priorities are stored in the most-significant bits
 * of the 8-bit NVIC priority register. Priority 0 is the highest and is NOT
 * maskable via BASEPRI, so configMAX_SYSCALL_INTERRUPT_PRIORITY must be
 * non-zero.  The ARM_CM3 port asserts this at scheduler start-up.
 *
 * The values below assume 4 NVIC priority bits (16 levels), which is the most
 * common configuration for Cortex-M3 devices (e.g. STM32F10x):
 *   - configKERNEL_INTERRUPT_PRIORITY      = 15 << (8-4) = 0xF0  (lowest priority)
 *   - configMAX_SYSCALL_INTERRUPT_PRIORITY =  5 << (8-4) = 0x50  (priority 5)
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
