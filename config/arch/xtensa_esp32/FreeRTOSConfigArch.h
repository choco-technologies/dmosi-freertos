/*
 * Architecture-specific FreeRTOS configuration for GCC Xtensa ESP32 port.
 *
 * The GCC_XTENSA_ESP32 port targets Espressif ESP32 class Xtensa MCUs.
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

/* Xtensa FreeRTOS port requires the maximum syscall interrupt priority to
 * match the exception level used by the core. */
#ifndef configMAX_SYSCALL_INTERRUPT_PRIORITY
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY    XCHAL_EXCM_LEVEL
#endif

#ifndef configMAX_API_CALL_INTERRUPT_PRIORITY
    #define configMAX_API_CALL_INTERRUPT_PRIORITY    configMAX_SYSCALL_INTERRUPT_PRIORITY
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
