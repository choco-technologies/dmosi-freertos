# Patches of the FreeRTOS-Kernel (lib/freertos)

`lib/freertos` is the upstream FreeRTOS-Kernel. Its Xtensa port for the ESP32 (`portable/ThirdParty/GCC/Xtensa_ESP32`) needs
changes to build with the kernel of this version and to link without ESP-IDF. They are not in the submodule (the upstream is
not ours to push to); apply them on a fresh checkout of it:

    git -C lib/freertos apply ../../patches/freertos-xtensa-esp32.patch

- `freertos-xtensa-esp32.patch`: `pxPortInitialiseStack` takes the MPU settings that the kernel passes since V11 (the port keeps
  the pointer to the coprocessor save area there), the port's `portmacro.h` / `xtensa_rtos.h` do not depend on the heap of
  ESP-IDF, and the port is in the list of the sources of `portable/CMakeLists.txt`.
  The `include/xtensa_context.h` and `include/xtensa_api.h` of the port only forwarded to `<xtensa/xtensa_context.h>` /
  `<xtensa/xtensa_api.h>`, which ESP-IDF >= 5.2 keeps in a `deprecated_include` directory that is not on the include path: they
  forward to `<xtensa_context.h>` / `<xtensa_api.h>` of the component xtensa (`#include_next`) when there are such headers. The
  other headers that ESP-IDF 5.2 removed or renamed (`soc/cpu.h`, `soc/spinlock.h`, `soc/compare_set.h`, `hal/cpu_hal.h`,
  `ESP_WATCHPOINT_STORE`) are covered by `config/arch/xtensa_esp32/` (it is on the include path of the port, before the headers
  of ESP-IDF; each of those headers includes the original one when the ESP-IDF has it, so an older ESP-IDF still works).
  `xtensa_loadstore_handler.S` (the emulation of the 8/16-bit loads and stores that the bus does not do, from ESP-IDF): the
  range of the addresses it accepts is the IRAM and the flash seen by the instruction bus (`SOC_IROM_LOW`..`SOC_IROM_HIGH`),
  where a 32-bit read stands in for the 8/16-bit one - without it, code that reads the bytes of an address of a function (a
  signature check on a pointer that may be a function) faults; the constants of the ESP32's APP cache (not on the ESP32-S3) are
  gone. The handler is built in with `CONFIG_ESP32_IRAM_AS_8BIT_ACCESSIBLE_MEMORY` and needs the symbol `_iram_text_end` (the end
  of the IRAM code); dmod-boot sets both (`src/arch/xtensa/lx7/shim/sdkconfig.h`, `configs/mcu/esp32s3/*/sections.ld`).
