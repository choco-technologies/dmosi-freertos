#ifndef DMOSI_FREERTOS_INTERNAL_H
#define DMOSI_FREERTOS_INTERNAL_H

/**
 * @file dmosi_freertos_internal.h
 * @brief Functions shared between the translation units of dmosi-freertos
 *
 * Nothing in here is part of the dmosi API.
 */
#include <stdbool.h>
#include "dmosi.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Check whether a process is the system process created by dmosi_init()
 *
 * The system process owns the threads the firmware itself runs on. A fault
 * in one of them cannot be contained: there is nothing left to return to.
 *
 * @param process Process to check
 * @return bool true if @p process is the system process
 */
bool dmosi_freertos_is_system_process(dmosi_process_t process);

/**
 * @brief Handle a stack overflow the kernel detected in a task
 *
 * Called from the kernel's stack overflow hook, i.e. from within a context
 * switch. Arranges for the process of the task to be terminated with a
 * DMOSI_FAULT_STACK_OVERFLOW fault, if the overflow can be contained to it
 * (see dmosi_thread_fault() for when it cannot; it also cannot before the
 * fault supervisor is started).
 *
 * @param task Task whose stack overflowed
 * @return bool true if the overflow is contained, false if it is fatal to
 *         the whole system
 */
bool dmosi_thread_stack_overflowed(TaskHandle_t task);

/**
 * @brief Start the fault supervisor: the thread that terminates the process
 *        of a thread caught overflowing its stack
 *
 * @param process Process the supervisor belongs to (the system process)
 * @return bool true on success, false if it could not be created
 */
bool dmosi_thread_fault_supervisor_start(dmosi_process_t process);

/**
 * @brief Stop the fault supervisor and release what it used
 *
 * After this a stack overflow is fatal again, as it is before the
 * supervisor is started.
 */
void dmosi_thread_fault_supervisor_stop(void);

#endif /* DMOSI_FREERTOS_INTERNAL_H */
