/*
 * Minimal esp_idf_version.h stub for standalone FreeRTOS builds outside
 * ESP-IDF.
 *
 * The GCC_XTENSA_ESP32 FreeRTOS port sources include this header and guard
 * code behind version comparisons.  When building without the ESP-IDF build
 * system, this stub is used as a fallback.
 *
 * The version is intentionally set to 4.1.0 (< 4.2.0).  Starting from ESP-IDF
 * 4.2, the context-save/restore assembly (xtensa_context.S, portasm.S, …)
 * gained new code paths that reference XT_STK_TMP0/1/2 – offsets defined only
 * in Espressif's extended <xtensa/xtensa_context.h> from the ESP-IDF xtensa
 * component, not in the standalone Xtensa toolchain headers.  Selecting a
 * version below 4.2.0 causes the preprocessor to skip those code paths.
 * For call0-ABI builds (e.g. ESP32-S3 with -mabi=call0) the register-window
 * spilling code inside #ifndef __XTENSA_CALL0_ABI__ is also skipped, so the
 * resulting context save/restore is correct.
 *
 * In a real ESP-IDF build (via idf.py), the generated esp_idf_version.h is
 * placed earlier in the include path and shadows this file automatically.
 */

#ifndef ESP_IDF_VERSION_H
#define ESP_IDF_VERSION_H

/** Encode a version as a single 24-bit integer (same as ESP-IDF's macro). */
#define ESP_IDF_VERSION_VAL(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

/** Reported ESP-IDF version for standalone builds (< 4.2.0). */
#define ESP_IDF_VERSION    ESP_IDF_VERSION_VAL(4, 1, 0)

#endif /* ESP_IDF_VERSION_H */
