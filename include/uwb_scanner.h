/**
 * @file uwb_scanner.h
 * @brief UWB device scanner interface
 */

#ifndef UWB_SCANNER_H
#define UWB_SCANNER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Structure containing information about a discovered UWB device
 */
typedef struct {
    uint64_t device_addr;      /* EUI-64 address of the device */
    uint32_t timestamp_ms;     /* System timestamp when device was detected */
    float distance_cm;         /* Estimated distance in centimeters */
    float rssi_dbm;           /* Received signal strength in dBm */
    uint16_t fpp_index;       /* First path power index */
    float fpp_level;          /* First path power level */
    uint8_t channel;          /* UWB channel used */
    uint8_t prf;              /* Pulse repetition frequency (16 or 64 MHz) */
    uint8_t frame_quality;    /* Frame quality indicator (0-255) */
} uwb_device_info_t;

/**
 * @brief Callback function type for device discovery
 *
 * @param info Pointer to device information structure
 */
typedef void (*uwb_device_callback_t)(const uwb_device_info_t *info);

/**
 * @brief Initialize the UWB scanner
 *
 * @param callback Function to call when a device is discovered
 * @return 0 on success, negative error code otherwise
 */
int uwb_scanner_init(uwb_device_callback_t callback);

/**
 * @brief Start continuous scanning for UWB devices
 *
 * @return 0 on success, negative error code otherwise
 */
int uwb_scanner_start(void);

/**
 * @brief Stop UWB scanning
 *
 * @return 0 on success, negative error code otherwise
 */
int uwb_scanner_stop(void);

/**
 * @brief Check if scanner is currently active
 *
 * @return true if scanning, false otherwise
 */
bool uwb_scanner_is_active(void);

#endif /* UWB_SCANNER_H */
