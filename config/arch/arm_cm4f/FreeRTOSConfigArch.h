/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-M4F port.
 *
 * The ARM_CM4F port (portable/GCC/ARM_CM4F) supports 16-bit, 32-bit and
 * 64-bit tick types.
 *
 * The Cortex-M4F has a hardware FPU (FPv4-SP-D16). The port requires the
 * compiler to be configured with hardware floating-point support. Ensure
 * the toolchain uses: -mfpu=fpv4-sp-d16 -mfloat-abi=hard (or softfp).
 * This is configured automatically by the build system via
 * FREERTOS_ARCH_COMPILER_FLAGS when DMOSI_ARCH_FAMILY=cortex-m4f is set.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

/* 32-bit tick is the most efficient choice on a 32-bit processor. */
#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* Map FreeRTOS ARM Cortex-M4F interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the FreeRTOS-internal names. */
#define vPortSVCHandler        dmosi_syscall_handler
#define xPortPendSVHandler     dmosi_context_switch_handler
#define xPortSysTickHandler    dmosi_tick_handler

/* Interrupt priority configuration for ARM Cortex-M4F.
 *
 * On ARM Cortex-M, interrupt priorities are stored in the most-significant bits
 * of the 8-bit NVIC priority register. Priority 0 is the highest and is NOT
 * maskable via BASEPRI, so configMAX_SYSCALL_INTERRUPT_PRIORITY must be
 * non-zero.  The ARM_CM4F port asserts this at scheduler start-up.
 *
 * The values below assume 4 NVIC priority bits (16 levels), which is the most
 * common configuration for Cortex-M4F devices (e.g. STM32F4):
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
