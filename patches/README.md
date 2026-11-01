# Patches of the FreeRTOS-Kernel (lib/freertos)

`lib/freertos` is the upstream FreeRTOS-Kernel. Its Xtensa port for the ESP32 (`portable/ThirdParty/GCC/Xtensa_ESP32`) needs
changes to build with the kernel of this version and to link without ESP-IDF. They are not in the submodule (the upstream is
not ours to push to); apply them on a fresh checkout of it:

    git -C lib/freertos apply ../../patches/freertos-xtensa-esp32.patch

- `freertos-xtensa-esp32.patch`: `pxPortInitialiseStack` takes the MPU settings that the kernel passes since V11 (the port keeps
  the pointer to the coprocessor save area there), the port's `portmacro.h` / `xtensa_rtos.h` do not depend on the heap of
  ESP-IDF, and the port is in the list of the sources of `portable/CMakeLists.txt`.
