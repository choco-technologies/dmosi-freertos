# =====================================================================
#   Architecture to FreeRTOS port mapping
#
#   Maps DMOSI_ARCH + DMOSI_ARCH_FAMILY + DMOSI_COMPILER to the
#   FREERTOS_PORT value expected by the FreeRTOS-Kernel CMake build.
#
#   Inputs (must be set before including this file):
#     DMOSI_ARCH         - Target architecture, e.g. "armv7"
#     DMOSI_ARCH_FAMILY  - Microcontroller family, e.g. "cortex-m7"
#     DMOSI_COMPILER     - Compiler (optional, defaults to "gcc")
#
#   Output:
#     FREERTOS_PORT      - FreeRTOS port string, e.g. "GCC_ARM_CM7"
# =====================================================================

if(NOT DEFINED DMOSI_ARCH OR NOT DEFINED DMOSI_ARCH_FAMILY)
    message(FATAL_ERROR
        "Both DMOSI_ARCH and DMOSI_ARCH_FAMILY must be defined before "
        "including arch_mapping.cmake.")
endif()

# Default compiler is GCC
if(NOT DEFINED DMOSI_COMPILER)
    set(DMOSI_COMPILER "gcc")
endif()

string(TOLOWER "${DMOSI_ARCH}"        _arch)
string(TOLOWER "${DMOSI_ARCH_FAMILY}" _family)
string(TOLOWER "${DMOSI_COMPILER}"    _compiler)

# -----------------------------------------------------------------------
# GCC port mappings
# -----------------------------------------------------------------------
if(_compiler STREQUAL "gcc")

    # --- ARM Cortex-M ---------------------------------------------------
    if(_family STREQUAL "cortex-m0" OR _family STREQUAL "cortex-m0+")
        set(FREERTOS_PORT "GCC_ARM_CM0")

    elseif(_family STREQUAL "cortex-m3")
        set(FREERTOS_PORT "GCC_ARM_CM3")

    elseif(_family STREQUAL "cortex-m4" OR _family STREQUAL "cortex-m4f")
        set(FREERTOS_PORT "GCC_ARM_CM4F")

    elseif(_family STREQUAL "cortex-m7")
        set(FREERTOS_PORT "GCC_ARM_CM7")

    elseif(_family STREQUAL "cortex-m23")
        set(FREERTOS_PORT "GCC_ARM_CM23_NONSECURE")

    elseif(_family STREQUAL "cortex-m33")
        set(FREERTOS_PORT "GCC_ARM_CM33_NONSECURE")

    elseif(_family STREQUAL "cortex-m35p")
        set(FREERTOS_PORT "GCC_ARM_CM35P_NONSECURE")

    elseif(_family STREQUAL "cortex-m52")
        set(FREERTOS_PORT "GCC_ARM_CM52_NONSECURE")

    elseif(_family STREQUAL "cortex-m55")
        set(FREERTOS_PORT "GCC_ARM_CM55_NONSECURE")

    elseif(_family STREQUAL "cortex-m85")
        set(FREERTOS_PORT "GCC_ARM_CM85_NONSECURE")

    # --- ARM Cortex-A ---------------------------------------------------
    elseif(_family STREQUAL "cortex-a9")
        set(FREERTOS_PORT "GCC_ARM_CA9")

    elseif(_family STREQUAL "aarch64" OR _family STREQUAL "cortex-a53"
            OR _family STREQUAL "cortex-a55" OR _family STREQUAL "cortex-a57"
            OR _family STREQUAL "cortex-a72" OR _family STREQUAL "cortex-a73"
            OR _family STREQUAL "cortex-a75" OR _family STREQUAL "cortex-a76"
            OR _arch STREQUAL "armv8-a" OR _arch STREQUAL "armv8")
        set(FREERTOS_PORT "GCC_ARM_AARCH64")

    # --- ARM Cortex-R ---------------------------------------------------
    elseif(_family STREQUAL "cortex-r4")
        set(FREERTOS_PORT "GCC_ARM_CRX_NOGIC")

    elseif(_family STREQUAL "cortex-r5")
        set(FREERTOS_PORT "GCC_ARM_CR5")

    elseif(_family STREQUAL "cortex-r82")
        set(FREERTOS_PORT "GCC_ARM_CR82")

    # --- ARM7 -----------------------------------------------------------
    elseif(_family STREQUAL "arm7tdmi" OR _arch STREQUAL "armv4t")
        set(FREERTOS_PORT "GCC_ARM7_LPC2000")

    # --- RISC-V ---------------------------------------------------------
    elseif(_arch STREQUAL "risc-v" OR _arch STREQUAL "riscv"
            OR _arch STREQUAL "rv32" OR _arch STREQUAL "rv64"
            OR _family STREQUAL "risc-v" OR _family STREQUAL "riscv"
            OR _family MATCHES "^rv32" OR _family MATCHES "^rv64")
        set(FREERTOS_PORT "GCC_RISC_V_GENERIC")

    # --- POSIX (host / simulation) --------------------------------------
    elseif(_family STREQUAL "posix" OR _arch STREQUAL "posix")
        set(FREERTOS_PORT "GCC_POSIX")

    else()
        message(FATAL_ERROR
            "No GCC FreeRTOS port mapping found for "
            "DMOSI_ARCH='${DMOSI_ARCH}' DMOSI_ARCH_FAMILY='${DMOSI_ARCH_FAMILY}'. "
            "Set FREERTOS_PORT manually or add a mapping to cmake/arch_mapping.cmake.")
    endif()

# -----------------------------------------------------------------------
# IAR port mappings
# -----------------------------------------------------------------------
elseif(_compiler STREQUAL "iar")

    if(_family STREQUAL "cortex-m0" OR _family STREQUAL "cortex-m0+")
        set(FREERTOS_PORT "IAR_ARM_CM0")

    elseif(_family STREQUAL "cortex-m3")
        set(FREERTOS_PORT "IAR_ARM_CM3")

    elseif(_family STREQUAL "cortex-m4" OR _family STREQUAL "cortex-m4f")
        set(FREERTOS_PORT "IAR_ARM_CM4F")

    elseif(_family STREQUAL "cortex-m7")
        set(FREERTOS_PORT "IAR_ARM_CM7")

    elseif(_family STREQUAL "cortex-m23")
        set(FREERTOS_PORT "IAR_ARM_CM23_NONSECURE")

    elseif(_family STREQUAL "cortex-m33")
        set(FREERTOS_PORT "IAR_ARM_CM33_NONSECURE")

    else()
        message(FATAL_ERROR
            "No IAR FreeRTOS port mapping found for "
            "DMOSI_ARCH='${DMOSI_ARCH}' DMOSI_ARCH_FAMILY='${DMOSI_ARCH_FAMILY}'. "
            "Set FREERTOS_PORT manually or add a mapping to cmake/arch_mapping.cmake.")
    endif()

else()
    message(FATAL_ERROR
        "Unsupported compiler '${DMOSI_COMPILER}'. "
        "Supported values: gcc, iar. "
        "Set FREERTOS_PORT manually or add a mapping to cmake/arch_mapping.cmake.")
endif()

message(STATUS
    "Arch mapping: DMOSI_ARCH='${DMOSI_ARCH}' "
    "DMOSI_ARCH_FAMILY='${DMOSI_ARCH_FAMILY}' "
    "DMOSI_COMPILER='${DMOSI_COMPILER}' "
    "-> FREERTOS_PORT='${FREERTOS_PORT}'")
