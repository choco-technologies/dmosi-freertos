# =====================================================================
#               DMOD OS Interface - FreeRTOS Integration
# =====================================================================
cmake_minimum_required(VERSION 3.10)

# ======================================================================
#               DMOD VFS
# ======================================================================
project(dmosi_freertos 
    VERSION 1.0
    DESCRIPTION "DMOD OS Interface for FreeRTOS"
    LANGUAGES C CXX)

# ======================================================================
#               For VS Code
# ======================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================================================================
#               Fetch DMOD repository
# ======================================================================
include(FetchContent)

# Only fetch and build dmod if it's not already available as a target
if(NOT TARGET dmod_inc)
    FetchContent_Declare(
        dmod
        GIT_REPOSITORY https://github.com/choco-technologies/dmod.git
        GIT_TAG        develop
    )

    # ======================================================================
    #               DMOD Configuration
    # ======================================================================
    set(DMOD_MODE "DMOD_SYSTEM" CACHE STRING "DMOD build mode")
    set(DMOD_BUILD_TESTS OFF CACHE BOOL "Build tests")
    set(DMOD_BUILD_EXAMPLES OFF CACHE BOOL "Build examples")
    set(DMOD_BUILD_TOOLS OFF CACHE BOOL "Build tools")
    set(DMOD_BUILD_TEMPLATES OFF CACHE BOOL "Build templates")
    set(DMOD_EXTERNAL_REGISTRATION ON CACHE BOOL "Use external registration")
    
    # Pass coverage flags to DMOD if enabled
    if(ENABLE_COVERAGE)
        set(DMOD_ENABLE_COVERAGE ON CACHE BOOL "Enable DMOD coverage")
    endif()

    FetchContent_MakeAvailable(dmod)
    set(DMOD_DIR ${dmod_SOURCE_DIR} CACHE PATH "DMOD source directory" FORCE)
else()
    message(STATUS "dmod target already exists, skipping FetchContent")
endif()

# ======================================================================
#               Fetch DMOD OS Interface
# ======================================================================
FetchContent_Declare(
    dmosi
    GIT_REPOSITORY
    https://github.com/choco-technologies/dmosi.git
    GIT_TAG        main
)

# ======================================================================
#               DMOD OS Interface Configuration
# ======================================================================
set(DMOD_DIR ${dmod_SOURCE_DIR})
FetchContent_MakeAvailable(dmosi)

# ======================================================================
#               FreeRTOS Configuration
# ======================================================================

# Generic DMOSI build parameters for clock and tick frequencies
set(DMOSI_CPU_CLOCK_HZ 20000000 CACHE STRING "CPU clock frequency in Hz")
set(DMOSI_TICK_RATE_HZ 100 CACHE STRING "Tick rate frequency in Hz")
set(DMOSI_TICK_TYPE_WIDTH_IN_BITS "TICK_TYPE_WIDTH_32_BITS" CACHE STRING
    "Tick type width for FreeRTOS (TICK_TYPE_WIDTH_16_BITS, TICK_TYPE_WIDTH_32_BITS, or TICK_TYPE_WIDTH_64_BITS)")

add_library(freertos_config INTERFACE)

target_include_directories(freertos_config
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
)

target_compile_definitions(freertos_config
    INTERFACE
    DMOSI_CPU_CLOCK_HZ=${DMOSI_CPU_CLOCK_HZ}
    DMOSI_TICK_RATE_HZ=${DMOSI_TICK_RATE_HZ}
    DMOSI_TICK_TYPE_WIDTH_IN_BITS=${DMOSI_TICK_TYPE_WIDTH_IN_BITS}
)

# Link with dmod and dmosi to get their include directories transitively
target_link_libraries(freertos_config
    INTERFACE
    dmod
    dmosi
)

# ======================================================================
#               Architecture Selection
# ======================================================================
set(DMOSI_ARCH        "" CACHE STRING "Architecture of the target microcontroller (e.g. armv7)")
set(DMOSI_ARCH_FAMILY "" CACHE STRING "Microcontroller family (e.g. cortex-m7)")
set(DMOSI_COMPILER    "gcc" CACHE STRING "Compiler toolchain used for FreeRTOS port selection (default: gcc)")

# Derive FREERTOS_PORT from DMOSI_ARCH + DMOSI_ARCH_FAMILY when the user
# has supplied them but has not set FREERTOS_PORT explicitly.
if(DMOSI_ARCH AND DMOSI_ARCH_FAMILY AND NOT DEFINED FREERTOS_PORT)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/arch_mapping.cmake)
endif()

# Fallback: use POSIX port for host/test builds when nothing was specified
if(NOT DEFINED FREERTOS_PORT)
    set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "FreeRTOS port")
    message(STATUS "FREERTOS_PORT not set, using default: ${FREERTOS_PORT}")
endif()

# Add FreeRTOS subdirectory
add_subdirectory(lib/freertos)

# ======================================================================
#               DMOSI Process Management
# ======================================================================
add_subdirectory(lib/dmosi-proc)

# ======================================================================
#               DMOD VFS Library definition
# ======================================================================
add_library(dmosi_freertos STATIC
    src/dmosi_freertos.c
    src/dmosi_mutex.c
    src/dmosi_thread.c
    src/dmosi_semaphore.c
    src/dmosi_queue.c
    src/dmosi_heap.c
    src/dmosi_timer.c
)

target_include_directories(dmosi_freertos PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/freertos/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
)

target_link_libraries(dmosi_freertos PUBLIC
    dmod
    dmosi
    dmosi_proc
    freertos_kernel
)

# Define version string for the library
target_compile_definitions(dmosi_freertos PRIVATE
    DMOSI_FREERTOS_VERSION="${PROJECT_VERSION}"
)

# ======================================================================
#               Tests
# ======================================================================
option(DMOSI_FREERTOS_BUILD_TESTS "Build tests" OFF)

if(DMOSI_FREERTOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
