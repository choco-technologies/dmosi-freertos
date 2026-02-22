/*
 * Architecture-specific FreeRTOS configuration for GCC AArch64 port.
 *
 * The GCC_ARM_AARCH64 port targets 64-bit ARM processors (ARMv8-A).
 * 64-bit tick type is appropriate for a 64-bit processor.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_64_BITS
#endif

/* Map FreeRTOS AArch64 interrupt handler names to the dmosi system interrupt
 * interface.  FreeRTOS_SWI_Handler services the SVC exception (supervisor
 * call) and FreeRTOS_Tick_Handler drives the RTOS time base. */
#define FreeRTOS_SWI_Handler     dmosi_syscall_handler
#define FreeRTOS_Tick_Handler    dmosi_tick_handler

#endif /* FREERTOS_CONFIG_ARCH_H */
