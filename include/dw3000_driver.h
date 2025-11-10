/**
 * @file dw3000_driver.h
 * @brief DW3000 UWB transceiver driver interface
 */

#ifndef DW3000_DRIVER_H
#define DW3000_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* DW3000 Register addresses */
#define DW3000_REG_DEV_ID           0x00
#define DW3000_REG_SYS_CFG          0x04
#define DW3000_REG_TX_FCTRL         0x08
#define DW3000_REG_RX_FINFO         0x10
#define DW3000_REG_RX_BUFFER        0x11
#define DW3000_REG_RX_FQUAL         0x12
#define DW3000_REG_RX_TTCKI         0x13
#define DW3000_REG_RX_TIME          0x15
#define DW3000_REG_TX_TIME          0x17
#define DW3000_REG_SYS_STATUS       0x44
#define DW3000_REG_RX_FINFO         0x10
#define DW3000_REG_CIA_CONF         0x50
#define DW3000_REG_IP_CONF          0x52

/* DW3000 Configuration */
#define DW3000_CHANNEL_5            5
#define DW3000_CHANNEL_9            9
#define DW3000_PRF_16M              1
#define DW3000_PRF_64M              2
#define DW3000_PLEN_64              0x01
#define DW3000_PLEN_128             0x05
#define DW3000_PLEN_256             0x09

/* Status flags */
#define DW3000_STATUS_RXFCG         (1 << 13)  /* Receiver FCS Good */
#define DW3000_STATUS_RXFCE         (1 << 15)  /* Receiver FCS Error */
#define DW3000_STATUS_RXRFTO        (1 << 17)  /* Receiver Frame Wait Timeout */
#define DW3000_STATUS_RXPTO         (1 << 21)  /* Preamble Timeout */
#define DW3000_STATUS_RXFR          (1 << 13)  /* Frame Ready */

/**
 * @brief DW3000 configuration structure
 */
typedef struct {
    uint8_t channel;           /* UWB channel (5 or 9) */
    uint8_t prf;               /* Pulse repetition frequency */
    uint8_t preamble_length;   /* Preamble length code */
    uint8_t pac_size;          /* Preamble acquisition chunk size */
    uint16_t tx_preamble_code; /* TX preamble code */
    uint16_t rx_preamble_code; /* RX preamble code */
} dw3000_config_t;

/**
 * @brief Structure for received frame information
 */
typedef struct {
    uint8_t buffer[127];       /* Frame data buffer */
    uint16_t length;           /* Frame length */
    uint64_t timestamp;        /* RX timestamp */
    float rssi;                /* Received signal strength */
    uint16_t fpp_index;        /* First path power index */
    float fpp_level;           /* First path power level */
    uint8_t frame_quality;     /* Quality indicator */
} dw3000_rx_frame_t;

/**
 * @brief Initialize DW3000 chip
 *
 * @return 0 on success, negative error code otherwise
 */
int dw3000_init(void);

/**
 * @brief Configure DW3000 for operation
 *
 * @param config Pointer to configuration structure
 * @return 0 on success, negative error code otherwise
 */
int dw3000_configure(const dw3000_config_t *config);

/**
 * @brief Enable receiver mode
 *
 * @param timeout_ms Receive timeout in milliseconds (0 for no timeout)
 * @return 0 on success, negative error code otherwise
 */
int dw3000_rx_enable(uint32_t timeout_ms);

/**
 * @brief Read received frame
 *
 * @param frame Pointer to frame structure to fill
 * @return 0 on success, negative error code otherwise
 */
int dw3000_read_frame(dw3000_rx_frame_t *frame);

/**
 * @brief Check if frame is ready to be read
 *
 * @return true if frame ready, false otherwise
 */
bool dw3000_is_frame_ready(void);

/**
 * @brief Get device ID
 *
 * @return 32-bit device ID
 */
uint32_t dw3000_get_device_id(void);

/**
 * @brief Set device address (EUI-64)
 *
 * @param addr 64-bit device address
 * @return 0 on success, negative error code otherwise
 */
int dw3000_set_device_address(uint64_t addr);

/**
 * @brief Perform soft reset
 *
 * @return 0 on success, negative error code otherwise
 */
int dw3000_reset(void);

/**
 * @brief Read register value
 *
 * @param reg Register address
 * @param data Buffer for data
 * @param len Length to read
 * @return 0 on success, negative error code otherwise
 */
int dw3000_read_reg(uint16_t reg, uint8_t *data, uint16_t len);

/**
 * @brief Write register value
 *
 * @param reg Register address
 * @param data Data to write
 * @param len Length to write
 * @return 0 on success, negative error code otherwise
 */
int dw3000_write_reg(uint16_t reg, const uint8_t *data, uint16_t len);

#endif /* DW3000_DRIVER_H */
