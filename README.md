# dmosi-freertos

FreeRTOS backend implementation of the [DMOSI](https://github.com/choco-technologies/dmosi) (DMOD OS Interface) — a portable OS-abstraction layer used by the [DMOD](https://github.com/choco-technologies/dmod) framework.

## Overview

`dmosi-freertos` maps the DMOSI abstract OS primitives onto FreeRTOS kernel services. It lets any DMOD module written against the DMOSI interface run on a FreeRTOS-based embedded system without modification.

The library provides FreeRTOS implementations for:

| Primitive | DMOSI API prefix | Backed by |
|-----------|-----------------|-----------|
| Threads | `dmosi_thread_*` | `xTaskCreate` / `vTaskDelete` |
| Mutexes | `dmosi_mutex_*` | `xSemaphoreCreateMutex` / `xSemaphoreCreateRecursiveMutex` |
| Semaphores | `dmosi_semaphore_*` | `xSemaphoreCreateCounting` |
| Queues | `dmosi_queue_*` | `xQueueCreate` |
| Timers | `dmosi_timer_*` | `xTimerCreate` |
| Heap | `pvPortMalloc` / `vPortFree` | DMOD allocator (`Dmod_MallocEx`) |

## Dependencies

| Dependency | How it is fetched |
|------------|------------------|
| [dmod](https://github.com/choco-technologies/dmod) | CMake `FetchContent` (branch `develop`) |
| [dmosi](https://github.com/choco-technologies/dmosi) | CMake `FetchContent` (branch `main`) |
| [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) | Git submodule (`lib/freertos`) |
| [dmosi-proc](https://github.com/choco-technologies/dmosi-proc) | Git submodule (`lib/dmosi-proc`) |

## Repository structure

```
dmosi-freertos/
├── config/          # FreeRTOSConfig.h — adjust for your target
├── lib/
│   ├── freertos/    # FreeRTOS-Kernel submodule
│   └── dmosi-proc/  # Process management submodule
├── src/             # DMOSI FreeRTOS implementation sources
│   ├── dmosi_freertos.c   # Init / deinit
│   ├── dmosi_thread.c
│   ├── dmosi_mutex.c
│   ├── dmosi_semaphore.c
│   ├── dmosi_queue.c
│   ├── dmosi_timer.c
│   └── dmosi_heap.c
└── tests/           # Integration tests (POSIX port)
```

## Building

### Prerequisites

- CMake ≥ 3.10
- A C/C++ cross-compiler (or the host GCC/Clang for the POSIX port)
- Git (submodules must be initialised)

### Clone with submodules

```bash
git clone --recurse-submodules https://github.com/choco-technologies/dmosi-freertos.git
```

If the repository was already cloned without submodules:

```bash
git submodule update --init --recursive
```

### Standalone build (POSIX port — for development / testing)

```bash
cmake -B build -DFREERTOS_PORT=GCC_POSIX
cmake --build build
```

### Build with tests

```bash
cmake -B build -DFREERTOS_PORT=GCC_POSIX -DDMOSI_FREERTOS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Cross-compilation

Set `FREERTOS_PORT` to the port that matches your target hardware, for example:

```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=<your-toolchain>.cmake \
  -DFREERTOS_PORT=GCC_ARM_CM4F
cmake --build build
```

Refer to the [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) documentation for available ports.

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `FREERTOS_PORT` | `GCC_POSIX` | FreeRTOS port to compile |
| `DMOSI_FREERTOS_BUILD_TESTS` | `OFF` | Build the integration tests |
| `ENABLE_COVERAGE` | — | Enable code-coverage instrumentation |

## Integration into your project

Add `dmosi-freertos` as a subdirectory or via `FetchContent`, then link against the `dmosi_freertos` target:

```cmake
add_subdirectory(dmosi-freertos)

target_link_libraries(my_app PRIVATE dmosi_freertos)
```

### Initialisation

Call `dmosi_init()` once at application startup (from within the initial running task) before using any other DMOSI API. Call `dmosi_deinit()` on shutdown to release resources. All application logic — threads, queues, timers, synchronisation — is expressed exclusively through DMOSI APIs:

```c
#include "dmosi.h"

static void worker(void *arg)
{
    dmosi_mutex_t mtx = (dmosi_mutex_t)arg;
    dmosi_mutex_lock(mtx);
    /* do work */
    dmosi_mutex_unlock(mtx);
}

void app_entry(void *arg)
{
    if (!dmosi_init()) {
        /* handle error */
        return;
    }

    dmosi_mutex_t mtx = dmosi_mutex_create(false);
    dmosi_thread_t t   = dmosi_thread_create(worker, mtx, 1, 4096, "worker", NULL);

    dmosi_thread_join(t);
    dmosi_thread_destroy(t);
    dmosi_mutex_destroy(mtx);

    dmosi_deinit();
}
```

`dmosi_init()` creates an internal *system* process and bootstraps thread-local storage for the calling task. `dmosi_deinit()` reverses this on shutdown.

## API summary

All functions below are accessed through the DMOSI interface macros defined in `dmosi.h`.

### Lifecycle

```c
bool dmosi_init(void);    // initialise the backend
bool dmosi_deinit(void);  // clean up the backend
```

### Threads

```c
dmosi_thread_t dmosi_thread_create(dmosi_thread_entry_t entry, void* arg,
                                   int priority, size_t stack_size,
                                   const char* name, dmosi_process_t process);
void           dmosi_thread_destroy(dmosi_thread_t thread);
int            dmosi_thread_join(dmosi_thread_t thread);
int            dmosi_thread_kill(dmosi_thread_t thread, int status);
dmosi_thread_t dmosi_thread_current(void);
void           dmosi_thread_sleep(uint32_t ms);
const char*    dmosi_thread_get_name(dmosi_thread_t thread);
int            dmosi_thread_get_priority(dmosi_thread_t thread);
dmosi_process_t dmosi_thread_get_process(dmosi_thread_t thread);
size_t         dmosi_thread_get_all(dmosi_thread_t* threads, size_t max_count);
size_t         dmosi_thread_get_by_process(dmosi_process_t process,
                                           dmosi_thread_t* threads, size_t max_count);
```

### Mutexes

```c
dmosi_mutex_t dmosi_mutex_create(bool recursive);
void          dmosi_mutex_destroy(dmosi_mutex_t mutex);
int           dmosi_mutex_lock(dmosi_mutex_t mutex);    // blocks until acquired
int           dmosi_mutex_unlock(dmosi_mutex_t mutex);
```

### Semaphores

```c
dmosi_semaphore_t dmosi_semaphore_create(uint32_t initial_count, uint32_t max_count);
void              dmosi_semaphore_destroy(dmosi_semaphore_t semaphore);
int               dmosi_semaphore_wait(dmosi_semaphore_t semaphore, int32_t timeout_ms);
int               dmosi_semaphore_post(dmosi_semaphore_t semaphore);
```

`timeout_ms` values: `0` = non-blocking, `-1` = wait forever, `> 0` = millisecond timeout.

### Queues

```c
dmosi_queue_t dmosi_queue_create(size_t item_size, uint32_t queue_length);
void          dmosi_queue_destroy(dmosi_queue_t queue);
int           dmosi_queue_send(dmosi_queue_t queue, const void* item, int32_t timeout_ms);
int           dmosi_queue_receive(dmosi_queue_t queue, void* item, int32_t timeout_ms);
```

### Timers

```c
dmosi_timer_t dmosi_timer_create(dmosi_timer_callback_t callback, void* arg,
                                 uint32_t period_ms, bool auto_reload);
void          dmosi_timer_destroy(dmosi_timer_t timer);
int           dmosi_timer_start(dmosi_timer_t timer);
int           dmosi_timer_stop(dmosi_timer_t timer);
int           dmosi_timer_reset(dmosi_timer_t timer);
int           dmosi_timer_set_period(dmosi_timer_t timer, uint32_t period_ms);
uint32_t      dmosi_timer_get_period(dmosi_timer_t timer);
```

## Heap integration

`dmosi-freertos` replaces the standard FreeRTOS heap implementation (`heap_*.c`) with a thin adapter that forwards all allocations to the DMOD memory allocator (`Dmod_MallocEx` / `Dmod_Free`). This allows DMOD's memory tracking and diagnostics to cover every allocation made by the FreeRTOS kernel.

Do **not** add any `heap_*.c` file from the FreeRTOS portable layer when using this library.

## License

This project is licensed under the terms found in [LICENSE](LICENSE).
