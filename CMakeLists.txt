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

# ======================================================================
#               Architecture Selection
# ======================================================================
set(DMOSI_ARCH        "" CACHE STRING "Architecture of the target microcontroller (e.g. armv7)")
set(DMOSI_ARCH_FAMILY "" CACHE STRING "Microcontroller family (e.g. cortex-m7)")
set(DMOSI_COMPILER    "gcc" CACHE STRING "Compiler toolchain used for FreeRTOS port selection (default: gcc)")

# Derive FREERTOS_PORT from DMOSI_ARCH + DMOSI_ARCH_FAMILY when the user
# has supplied them but has not set FREERTOS_PORT explicitly.
# This also sets FREERTOS_ARCH_CONFIG_SUBDIR and FREERTOS_ARCH_COMPILER_FLAGS.
if(DMOSI_ARCH AND DMOSI_ARCH_FAMILY AND NOT DEFINED FREERTOS_PORT)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/arch_mapping.cmake)
endif()

# Fallback: use POSIX port for host/test builds when nothing was specified
if(NOT DEFINED FREERTOS_PORT)
    set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "FreeRTOS port")
    message(STATUS "FREERTOS_PORT not set, using default: ${FREERTOS_PORT}")
endif()

# For the GCC_POSIX port, always set the arch config subdir (whether the port
# was derived above or supplied explicitly on the command line).
if(FREERTOS_PORT STREQUAL "GCC_POSIX" AND NOT FREERTOS_ARCH_CONFIG_SUBDIR)
    set(FREERTOS_ARCH_CONFIG_SUBDIR "posix")
endif()

add_library(freertos_config INTERFACE)

# Include arch-specific config directory first so its FreeRTOSConfigArch.h
# is found before the fallback in config/FreeRTOSConfigArch.h.
if(FREERTOS_ARCH_CONFIG_SUBDIR)
    target_include_directories(freertos_config
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config/arch/${FREERTOS_ARCH_CONFIG_SUBDIR}>
    )
endif()

target_include_directories(freertos_config
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
)

target_compile_definitions(freertos_config
    INTERFACE
    DMOSI_CPU_CLOCK_HZ=${DMOSI_CPU_CLOCK_HZ}
    DMOSI_TICK_RATE_HZ=${DMOSI_TICK_RATE_HZ}
)

# Apply arch-specific compiler flags required by the selected FreeRTOS port
# (e.g. hardware FPU flags for ARM Cortex-M4F and Cortex-M7).
if(FREERTOS_ARCH_COMPILER_FLAGS)
    target_compile_options(freertos_config
        INTERFACE
        ${FREERTOS_ARCH_COMPILER_FLAGS}
    )
endif()

# Link with dmod and dmosi to get their include directories transitively
target_link_libraries(freertos_config
    INTERFACE
    dmod
    dmosi
)

# The ESP32 Xtensa FreeRTOS port requires Xtensa HAL headers (xtensa/hal.h).
# When building outside ESP-IDF component registration, add a best-effort
# fallback include path from IDF_PATH if available.
if(FREERTOS_PORT STREQUAL "GCC_XTENSA_ESP32")
    if(TARGET idf::xtensa)
        target_link_libraries(freertos_config INTERFACE idf::xtensa)
    endif()

    if(DEFINED ENV{IDF_PATH} AND NOT "$ENV{IDF_PATH}" STREQUAL "")
        set(_idf_path "$ENV{IDF_PATH}")
    elseif(DEFINED IDF_PATH AND NOT "${IDF_PATH}" STREQUAL "")
        set(_idf_path "${IDF_PATH}")
    endif()

    if(DEFINED _idf_path)
        set(DMOSI_IDF_PATH "${_idf_path}")

        set(_idf_esp32_include_dirs
            "${_idf_path}/components/xtensa/include"
            "${_idf_path}/components/esp_hw_support/include"
            "${_idf_path}/components/esp_hw_support/include/soc"
            "${_idf_path}/components/hal/include"
            "${_idf_path}/components/soc/include"
            "${_idf_path}/components/esp_rom/include"
            "${_idf_path}/components/esp_common/include"
            "${_idf_path}/components/esp_system/include"
            "${_idf_path}/components/esp_timer/include"
            "${_idf_path}/components/log/include"
            "${_idf_path}/components/heap/include"
        )

        if(DEFINED DMOD_CPU AND NOT "${DMOD_CPU}" STREQUAL "")
            list(APPEND _idf_esp32_include_dirs
                "${_idf_path}/components/xtensa/${DMOD_CPU}/include"
                "${_idf_path}/components/hal/${DMOD_CPU}/include"
                "${_idf_path}/components/soc/${DMOD_CPU}/include"
                "${_idf_path}/components/esp_rom/include/${DMOD_CPU}"
            )
        endif()

        foreach(_idf_include_dir IN LISTS _idf_esp32_include_dirs)
            if(EXISTS "${_idf_include_dir}")
                target_include_directories(freertos_config
                    INTERFACE
                    $<BUILD_INTERFACE:${_idf_include_dir}>
                )
            endif()
        endforeach()

        unset(_idf_include_dir)
        unset(_idf_esp32_include_dirs)
        unset(_idf_path)
    endif()

    target_compile_definitions(freertos_config
        INTERFACE
        portNUM_PROCESSORS=1
    )
endif()

# Add FreeRTOS subdirectory
add_subdirectory(lib/freertos)

if(FREERTOS_PORT STREQUAL "GCC_XTENSA_ESP32" AND DEFINED DMOSI_IDF_PATH)
    set(_dmosi_xtensa_asm_sources
        "${DMOSI_IDF_PATH}/components/xtensa/xtensa_intr_asm.S"
        "${DMOSI_IDF_PATH}/components/esp_system/port/arch/xtensa/panic_handler_asm.S"
    )

    foreach(_dmosi_xtensa_asm_source IN LISTS _dmosi_xtensa_asm_sources)
        if(EXISTS "${_dmosi_xtensa_asm_source}")
            target_sources(freertos_kernel_port PRIVATE "${_dmosi_xtensa_asm_source}")
        endif()
    endforeach()

    target_sources(freertos_kernel_port PRIVATE
        "${CMAKE_SOURCE_DIR}/src/arch/xtensa/xtensa_compat_stubs.c"
    )

    target_compile_definitions(freertos_kernel_port PRIVATE XT_SIMULATOR=1)

    unset(_dmosi_xtensa_asm_source)
    unset(_dmosi_xtensa_asm_sources)
endif()

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
    src/dmosi_time.c
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

# Treat warnings as errors for this project's sources
target_compile_options(dmosi_freertos PRIVATE -Wall -Wextra -Werror)

if(FREERTOS_PORT STREQUAL "GCC_XTENSA_ESP32")
    target_compile_options(dmosi_freertos PRIVATE -Wno-unused-parameter)
endif()

# ======================================================================
#               Tests
# ======================================================================
option(DMOSI_FREERTOS_BUILD_TESTS "Build tests" OFF)

if(DMOSI_FREERTOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
