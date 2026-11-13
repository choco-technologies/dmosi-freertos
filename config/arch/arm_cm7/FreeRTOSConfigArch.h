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

/* Interrupt priority configuration for ARM Cortex-M7.
 *
 * On ARM Cortex-M, interrupt priorities are stored in the most-significant bits
 * of the 8-bit NVIC priority register. Priority 0 is the highest and is NOT
 * maskable via BASEPRI, so configMAX_SYSCALL_INTERRUPT_PRIORITY must be
 * non-zero.  The ARM_CM7/r0p1 port asserts this at scheduler start-up.
 *
 * The values below assume 4 NVIC priority bits (16 levels), which is the most
 * common configuration for Cortex-M7 devices (e.g. STM32H7, STM32F7):
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

/* Map FreeRTOS ARM Cortex-M7 interrupt handler names to the dmosi system
 * interrupt interface.  This lets users install dmosi_syscall_handler,
 * dmosi_context_switch_handler, and dmosi_tick_handler directly in their
 * vector tables instead of the FreeRTOS-internal names. */
#define vPortSVCHandler        dmosi_syscall_handler
#define xPortPendSVHandler     dmosi_context_switch_handler
#define xPortSysTickHandler    dmosi_tick_handler

/* Move the stack guard of the architecture (dmosi_stack_guard_select, see dmosi.h) to the stack of the task that is
 * about to run: an overflow then faults at once instead of being found, after the damage, by the check of the kernel.
 * The first task of the scheduler is started without a context switch and runs unguarded until its first one.
 * The guard takes room in the stack: dmosi_thread_create() adds DMOSI_STACK_GUARD_RESERVE to the stacks it allocates. */
#ifndef __ASSEMBLER__
#include "dmosi.h"
/* pxEndOfStack is the last element of the buffer of a stack that grows down only when this is set */
#define configRECORD_STACK_HIGH_ADDRESS    1
#define traceTASK_SWITCHED_IN()                                                                                 \
    dmosi_stack_guard_select( ( const void * ) pxCurrentTCB->pxStack,                                          \
                              ( size_t ) ( ( pxCurrentTCB->pxEndOfStack - pxCurrentTCB->pxStack + 1 ) * sizeof( StackType_t ) ) )
/* Worst case: the guard itself, and what is lost aligning it to its size; the kernel's own pattern before it */
#define DMOSI_STACK_GUARD_RESERVE          ( 2 * DMOSI_STACK_GUARD_SIZE + 32 )
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
