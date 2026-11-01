/*
 * Architecture-specific FreeRTOS configuration for GCC Xtensa ESP32 port.
 *
 * The GCC_XTENSA_ESP32 port targets Espressif ESP32 class Xtensa MCUs. It brings its own part of the configuration
 * (FreeRTOSConfig_arch.h of the port: the interrupt stack, the timer of the core, the priority of the system calls); it reads
 * the Kconfig of ESP-IDF (sdkconfig.h), which the project that builds the port gives (a header of that name, with the options
 * the port reads: CONFIG_FREERTOS_UNICORE, CONFIG_FREERTOS_ISR_STACKSIZE, ...).
 */

#ifndef FREERTOS_CONFIG_ARCH_H
#define FREERTOS_CONFIG_ARCH_H

#ifndef DMOSI_TICK_TYPE_WIDTH_IN_BITS
    #define DMOSI_TICK_TYPE_WIDTH_IN_BITS    TICK_TYPE_WIDTH_32_BITS
#endif

#include "sdkconfig.h"
/* (ESP-IDF's FreeRTOSConfig.h defines it; the assembly of the port sizes its per-core data with it) */
#ifdef CONFIG_FREERTOS_UNICORE
    #define portNUM_PROCESSORS    1
#else
    #define portNUM_PROCESSORS    2
#endif

/* The port keeps the pointer to the coprocessor save area of a task in the TCB, where the kernel has the MPU settings (the
 * second member): its assembly reads it from offset 4 (CP_TOPOFSTACK_OFFS). There is no MPU. */
#define portUSING_MPU_WRAPPERS                      1
/* ... without the wrappers of the API that mpu_wrappers.h would define for a real MPU port (the guard of that file) */
#define MPU_WRAPPERS_H
#define PRIVILEGED_FUNCTION
#define PRIVILEGED_DATA
#define FREERTOS_SYSTEM_CALL
/* ... and the kernel does not give the memory of the idle and the timer task when the port is an MPU one (nothing here uses
 * the static allocation: dmosi creates everything from the heap) */
#define configSUPPORT_STATIC_ALLOCATION             0
#define configKERNEL_PROVIDED_STATIC_MEMORY         0

#include "FreeRTOSConfig_arch.h"

/* The stack of this port is counted in BYTES (StackType_t is uint8_t), not in words. The frame that the port builds on the stack
 * of a new task (the registers, the coprocessors: XT_STK_FRMSZ + XT_CP_SIZE) is about 600 bytes, and the windowed ABI spills 16..64
 * bytes per call: the idle task, which frees the stacks of the finished tasks, needs several kilobytes. */
#define configMINIMAL_STACK_SIZE                    4096
#define configTIMER_TASK_STACK_DEPTH                8192

/* What dmosi-freertos expects of a port and the port of Espressif calls differently or defines itself */
#define xPortIsInsideInterrupt()                    xPortInIsrContext()
/* The run time statistics: the counter of the core (the port defines them the same way; a different definition would clash) */
#define portGET_RUN_TIME_COUNTER_VALUE()            xthal_get_ccount()
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
/* The port has its own xPortGetFreeHeapSize() and xPortGetMinimumEverFreeHeapSize() (static inline, in portmacro.h) */
#define DMOSI_PORT_HAS_HEAP_STATS                   1
/* The tick handler of the port (port_systick.c) passes the number of the interrupt to the trace macro of the IDF */
#define traceISR_ENTER( ... )

#endif /* FREERTOS_CONFIG_ARCH_H */
