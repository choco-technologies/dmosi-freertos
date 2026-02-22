/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-R4 port.
 *
 * The GCC_ARM_CRX_NOGIC port targets Cortex-R4 (ARMv7-R).
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* Map FreeRTOS ARM Cortex-R4 interrupt handler names to the dmosi system
 * interrupt interface.  FreeRTOS_SVC_Handler services the SVC exception
 * (supervisor call) and FreeRTOS_Tick_Handler drives the RTOS time base. */
#define FreeRTOS_SVC_Handler     dmosi_syscall_handler
#define FreeRTOS_Tick_Handler    dmosi_tick_handler

#endif /* FREERTOS_CONFIG_ARCH_H */
