/*
 * Architecture-specific FreeRTOS configuration for GCC ARM Cortex-A9 port.
 *
 * The GCC_ARM_CA9 port supports 32-bit tick types.
 * Cortex-A9 is an ARMv7-A processor with hardware FPU.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* Map FreeRTOS ARM Cortex-A9 interrupt handler names to the dmosi system
 * interrupt interface.  FreeRTOS_SWI_Handler services the SWI/SVC exception
 * (supervisor call) and FreeRTOS_Tick_Handler drives the RTOS time base. */
#define FreeRTOS_SWI_Handler     dmosi_syscall_handler
#define FreeRTOS_Tick_Handler    dmosi_tick_handler

#endif /* FREERTOS_CONFIG_ARCH_H */
