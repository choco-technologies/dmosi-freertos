# dmosi-freertos

FreeRTOS backend implementation of the [dmosi](https://github.com/choco-technologies/dmosi) (DMOD OS Interface) abstraction layer. It maps the portable dmosi API (threads, mutexes, semaphores, queues, timers, and heap) to the corresponding FreeRTOS primitives and integrates with the [dmod](https://github.com/choco-technologies/dmod) module system.

## Features

- **Thread management** – create, destroy, join, sleep, kill, and enumerate threads backed by FreeRTOS tasks
- **Mutex** – regular and recursive mutexes via FreeRTOS semaphore API
- **Semaphore** – counting semaphores with configurable initial and maximum counts
- **Queue** – fixed-size message queues with blocking send/receive
- **Software timers** – one-shot and periodic timers with user callbacks
- **Heap** – custom `pvPortMalloc`/`vPortFree` that delegate to the dmod memory allocator for unified memory tracking

## Repository layout

```
.
├── cmake/
│   └── arch_mapping.cmake   # Maps DMOSI_ARCH + DMOSI_ARCH_FAMILY to a FREERTOS_PORT value
├── config/
│   └── FreeRTOSConfig.h     # FreeRTOS kernel configuration header
├── lib/
│   ├── freertos/            # FreeRTOS-Kernel (git submodule)
│   └── dmosi-proc/          # dmosi process management (git submodule)
├── src/
│   ├── dmosi_freertos.c     # Init / deinit entry points
│   ├── dmosi_thread.c       # Thread API
│   ├── dmosi_mutex.c        # Mutex API
│   ├── dmosi_semaphore.c    # Semaphore API
│   ├── dmosi_queue.c        # Queue API
│   ├── dmosi_timer.c        # Timer API
│   └── dmosi_heap.c         # Custom heap (pvPortMalloc / vPortFree)
├── tests/
│   └── main.c               # Integration tests (run via CTest)
└── CMakeLists.txt
```

## Requirements

- CMake ≥ 3.10
- A C11-capable compiler (GCC or IAR)
- Internet access for the CMake `FetchContent` step (downloads `dmod` and `dmosi` automatically)
- Git submodules initialised (`lib/freertos` and `lib/dmosi-proc`)

## Building

### 1. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/choco-technologies/dmosi-freertos.git
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

### 2. Configure

#### Host / POSIX build (default, useful for tests)

```bash
cmake -B build -DFREERTOS_PORT=GCC_POSIX
```

#### Embedded target build

Specify the target architecture so that `arch_mapping.cmake` can derive the correct FreeRTOS port automatically:

```bash
cmake -B build \
  -DDMOSI_ARCH=armv7 \
  -DDMOSI_ARCH_FAMILY=cortex-m7
```

Or set the FreeRTOS port directly:

```bash
cmake -B build -DFREERTOS_PORT=GCC_ARM_CM7
```

### 3. Build

```bash
cmake --build build
```

## CMake configuration options

| Option | Default | Description |
|---|---|---|
| `FREERTOS_PORT` | `GCC_POSIX` | FreeRTOS port to use (derived automatically from `DMOSI_ARCH` + `DMOSI_ARCH_FAMILY` when those are set) |
| `DMOSI_ARCH` | *(empty)* | Target architecture, e.g. `armv7` |
| `DMOSI_ARCH_FAMILY` | *(empty)* | Microcontroller family, e.g. `cortex-m7` |
| `DMOSI_COMPILER` | `gcc` | Compiler toolchain used for port selection (`gcc` or `iar`) |
| `DMOSI_CPU_CLOCK_HZ` | `20000000` | CPU clock frequency in Hz (passed to `FreeRTOSConfig.h`) |
| `DMOSI_TICK_RATE_HZ` | `100` | FreeRTOS tick rate in Hz (passed to `FreeRTOSConfig.h`) |
| `DMOSI_FREERTOS_BUILD_TESTS` | `OFF` | Build and register the CTest integration tests |

## Architecture / FreeRTOS port mapping

When `DMOSI_ARCH` and `DMOSI_ARCH_FAMILY` are both set and `FREERTOS_PORT` is not, `cmake/arch_mapping.cmake` automatically selects the correct port. Supported families include:

| `DMOSI_ARCH_FAMILY` | `FREERTOS_PORT` (GCC) |
|---|---|
| `cortex-m0`, `cortex-m0+` | `GCC_ARM_CM0` |
| `cortex-m3` | `GCC_ARM_CM3` |
| `cortex-m4`, `cortex-m4f` | `GCC_ARM_CM4F` |
| `cortex-m7` | `GCC_ARM_CM7` |
| `cortex-m23` | `GCC_ARM_CM23_NONSECURE` |
| `cortex-m33` | `GCC_ARM_CM33_NONSECURE` |
| `cortex-a9` | `GCC_ARM_CA9` |
| `aarch64` / `cortex-a53` … `cortex-a76` | `GCC_ARM_AARCH64` |
| `cortex-r4` | `GCC_ARM_CRX_NOGIC` |
| `cortex-r5` | `GCC_ARM_CR5` |
| RISC-V (`rv32*`, `rv64*`) | `GCC_RISC_V_GENERIC` |
| `posix` | `GCC_POSIX` |

IAR toolchain mappings are supported for Cortex-M0 through Cortex-M7 and Cortex-M23/M33. For any unsupported combination, set `FREERTOS_PORT` manually.

## Running tests

```bash
cmake -B build -DFREERTOS_PORT=GCC_POSIX -DDMOSI_FREERTOS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Using as a CMake dependency

Add this repository as a subdirectory or a `FetchContent` target in your project, then link against the `dmosi_freertos` target:

```cmake
target_link_libraries(my_app PRIVATE dmosi_freertos)
```

This transitively pulls in `dmod`, `dmosi`, `dmosi_proc`, and `freertos_kernel`.

## License

See [LICENSE](LICENSE) for details.
