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
    GIT_TAG        master
)

# ======================================================================
#               DMOD File System Interface Configuration
# ======================================================================
set(DMOD_DIR ${dmod_SOURCE_DIR})
FetchContent_MakeAvailable(dmosi)

# ======================================================================
#               DMOD VFS Library definition
# ======================================================================
add_library(dmosi_freertos STATIC
    src/dmosi_freertos.c
)

target_include_directories(dmosi_freertos PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(dmosi_freertos PUBLIC
    dmod
    dmosi
)

# Define version string for the library
target_compile_definitions(dmosi_freertos PRIVATE
    DMOSI_FREERTOS_VERSION="${PROJECT_VERSION}"
)
