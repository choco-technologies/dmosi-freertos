cmake_minimum_required(VERSION 3.10)

project(dmosi_freertos_tests
    VERSION 1.0
    DESCRIPTION "DMOSI FreeRTOS implementation tests"
    LANGUAGES C)

# Add the test executable
add_executable(${PROJECT_NAME} main.c)

# Link against dmosi_freertos (which pulls in dmod, dmosi, dmosi_proc, freertos_kernel).
# dmosi_freertos must come before dmosi so that its strong implementations
# are resolved by the linker before dmosi's weak fallbacks are selected.
target_link_libraries(${PROJECT_NAME}
    dmosi_freertos
    dmosi_proc
    dmosi
    dmod
    pthread
)

# Use the DMOD linker script so that .dmod.inputs / .dmod.outputs sections
# are properly placed in the binary.
target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_DIR}/scripts)
target_link_options(${PROJECT_NAME} PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/main.ld)

# Register test with CTest
add_test(NAME dmosi_freertos_tests COMMAND ${PROJECT_NAME})
