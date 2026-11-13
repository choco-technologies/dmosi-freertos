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

/* The check of the stack overflow of the kernel (stack_macros.h) is compiled out when portUSING_MPU_WRAPPERS is 1, as above, so the
 * overflow went unnoticed: it ran over the neighbour block of the heap. The same check as the kernel's for a stack that grows down
 * (configCHECK_FOR_STACK_OVERFLOW 2: the saved stack pointer within the limit, the pattern at the end of the stack untouched). */
#define taskCHECK_FOR_STACK_OVERFLOW()                                                       \
    do                                                                                       \
    {                                                                                        \
        const uint32_t * const pulStack = ( uint32_t * ) pxCurrentTCB->pxStack;              \
        const uint32_t ulCheckValue = ( uint32_t ) 0xa5a5a5a5U;                              \
                                                                                             \
        if( ( pxCurrentTCB->pxTopOfStack <= pxCurrentTCB->pxStack ) ||                       \
            ( pulStack[ 0 ] != ulCheckValue ) ||                                             \
            ( pulStack[ 1 ] != ulCheckValue ) ||                                             \
            ( pulStack[ 2 ] != ulCheckValue ) ||                                             \
            ( pulStack[ 3 ] != ulCheckValue ) )                                              \
        {                                                                                    \
            char * pcOverflowTaskName = pxCurrentTCB->pcTaskName;                            \
            vApplicationStackOverflowHook( ( TaskHandle_t ) pxCurrentTCB, pcOverflowTaskName ); \
        }                                                                                    \
    } while( 0 )

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

/* The guard of the stack of the thread that runs: the architecture is told the stack of every thread that is switched to
 * (dmosi_stack_guard_select(): dmod-boot puts the data breakpoints of the core on the lowest part of it, and an overflow
 * stops the thread the moment it writes there). The first task of the scheduler is started without a context switch and runs
 * unguarded until its first one. The guard takes room in the stack: dmosi_thread_create() adds DMOSI_STACK_GUARD_RESERVE to the
 * stacks it allocates. The stack is counted in bytes on this port. */
#ifndef __ASSEMBLER__
#include "dmosi.h"
/* pxEndOfStack is the last element of the buffer of a stack that grows down only when this is set */
#define configRECORD_STACK_HIGH_ADDRESS    1
#define traceTASK_SWITCHED_IN()                                                                                 \
    dmosi_stack_guard_select( ( const void * ) pxCurrentTCB->pxStack,                                          \
                              ( size_t ) ( ( pxCurrentTCB->pxEndOfStack - pxCurrentTCB->pxStack + 1 ) * sizeof( StackType_t ) ) )
/* The room to handle the overflow in below the guard, the guard (two data breakpoints), and what is lost aligning it: see STACK_GUARD_SLACK in fault.c */
#define DMOSI_STACK_GUARD_RESERVE          768
#endif

#endif /* FREERTOS_CONFIG_ARCH_H */
