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

/* The Xtensa ESP32 port's FreeRTOSConfig_arch.h unconditionally defines
 * configMAX_SYSCALL_INTERRUPT_PRIORITY to XCHAL_EXCM_LEVEL.  Pre-define it
 * here so that FreeRTOSConfig.h does not set it to the generic default of 0,
 * which would cause a redefinition warning and assembler errors in the Xtensa
 * context-save/restore code (xtensa_context.S).
 *
 * XCHAL_EXCM_LEVEL is provided by the Xtensa HAL (<xtensa/coreasm.h> for
 * assembler translation units and <xtensa/config/core.h> for C translation
 * units).  Both are included by xtensa_rtos.h before it includes
 * FreeRTOSConfig.h, so XCHAL_EXCM_LEVEL is always in scope when this file
 * is processed as part of a GCC_XTENSA_ESP32 build. */
#ifndef configMAX_SYSCALL_INTERRUPT_PRIORITY
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY    XCHAL_EXCM_LEVEL
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
