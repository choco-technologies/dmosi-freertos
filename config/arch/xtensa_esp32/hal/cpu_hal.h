/* ESP-IDF >= 5.2: cpu_hal_get_core_id() is a macro for cpu_ll_get_core_id(), which is deprecated (an error with -Werror);
   esp_cpu_get_core_id() of esp_cpu.h is what replaced it. The original header is used as it is when it does not have that macro
   (an older ESP-IDF). */
#pragma once
#if __has_include_next(<hal/cpu_hal.h>)
    #include_next <hal/cpu_hal.h>
#endif
#if defined(cpu_hal_get_core_id) && __has_include(<esp_cpu.h>)
    #include <esp_cpu.h>
    #undef cpu_hal_get_core_id
    #define cpu_hal_get_core_id()   esp_cpu_get_core_id()
#endif
