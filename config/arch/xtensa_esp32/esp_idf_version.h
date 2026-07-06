/*
 * Minimal esp_idf_version.h stub for standalone FreeRTOS builds outside
 * ESP-IDF.
 *
 * The GCC_XTENSA_ESP32 FreeRTOS port sources include this header and guard
 * code behind version comparisons.  When building without the ESP-IDF build
 * system, this stub is used as a fallback.
 *
 * This standalone profile is aligned with ESP-IDF 4.4.x used by the toolchain.
 * Matching the real IDF major/minor version ensures Xtensa FreeRTOS port code
 * selects compatible include paths and APIs for ESP32-S3.
 *
 * In a real ESP-IDF build (via idf.py), the generated esp_idf_version.h is
 * placed earlier in the include path and shadows this file automatically.
 */

#ifndef ESP_IDF_VERSION_H
#define ESP_IDF_VERSION_H

/** Encode a version as a single 24-bit integer (same as ESP-IDF's macro). */
#define ESP_IDF_VERSION_VAL(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

/** Reported ESP-IDF version for standalone builds. */
#define ESP_IDF_VERSION    ESP_IDF_VERSION_VAL(4, 4, 7)

#endif /* ESP_IDF_VERSION_H */
