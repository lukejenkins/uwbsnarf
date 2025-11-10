/**
 * @file dw3000_driver.c
 * @brief DW3000 UWB transceiver driver implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

#include "dw3000_driver.h"

LOG_MODULE_REGISTER(dw3000, LOG_LEVEL_INF);

/* SPI Configuration */
#define DW3000_SPI_NODE DT_NODELABEL(dw3000_spi)
#define DW3000_SPI_FREQ 8000000
#define DW3000_SPI_MODE (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

/* GPIO Configuration */
#define DW3000_RST_NODE DT_NODELABEL(dw3000_reset)
#define DW3000_IRQ_NODE DT_NODELABEL(dw3000_irq)

/* SPI Commands */
#define DW3000_SPI_WRITE 0x80
#define DW3000_SPI_READ  0x00

/* Device constants */
#define DW3000_DEVICE_ID 0xDECA0302

/* Static variables */
static const struct device *spi_dev;
static struct spi_config spi_cfg;
static struct spi_cs_control cs_ctrl;

/* Helper function to perform SPI transaction */
static int dw3000_spi_transfer(uint16_t reg, uint8_t *data, uint16_t len, bool write)
{
    uint8_t header[3];
    int header_len;

    /* Build SPI header */
    if (reg < 0x80) {
        /* Short register address */
        header[0] = (write ? DW3000_SPI_WRITE : DW3000_SPI_READ) | (reg & 0x3F);
        header_len = 1;
    } else {
        /* Extended register address */
        header[0] = (write ? DW3000_SPI_WRITE : DW3000_SPI_READ) | 0x40;
        header[1] = (reg & 0x7F);
        header[2] = ((reg >> 7) & 0xFF);
        header_len = 3;
    }

    struct spi_buf tx_bufs[] = {
        {.buf = header, .len = header_len},
        {.buf = write ? data : NULL, .len = write ? len : 0}
    };

    struct spi_buf rx_bufs[] = {
        {.buf = NULL, .len = header_len},
        {.buf = write ? NULL : data, .len = write ? 0 : len}
    };

    struct spi_buf_set tx = {.buffers = tx_bufs, .count = 2};
    struct spi_buf_set rx = {.buffers = rx_bufs, .count = 2};

    int ret = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
    if (ret < 0) {
        LOG_ERR("SPI transfer failed: %d", ret);
        return ret;
    }

    return 0;
}

int dw3000_read_reg(uint16_t reg, uint8_t *data, uint16_t len)
{
    return dw3000_spi_transfer(reg, data, len, false);
}

int dw3000_write_reg(uint16_t reg, const uint8_t *data, uint16_t len)
{
    return dw3000_spi_transfer(reg, (uint8_t *)data, len, true);
}

int dw3000_init(void)
{
    LOG_INF("Initializing DW3000");

    /* Get SPI device */
    spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready");
        return -ENODEV;
    }

    /* Configure SPI */
    spi_cfg.frequency = DW3000_SPI_FREQ;
    spi_cfg.operation = DW3000_SPI_MODE;
    spi_cfg.slave = 0;
    spi_cfg.cs = NULL; /* CS controlled by GPIO */

    /* Perform soft reset */
    k_sleep(K_MSEC(10));

    /* Read and verify device ID */
    uint32_t dev_id = dw3000_get_device_id();
    LOG_INF("Device ID: 0x%08X", dev_id);

    if ((dev_id & 0xFFFFFF00) != (DW3000_DEVICE_ID & 0xFFFFFF00)) {
        LOG_ERR("Invalid device ID: expected 0x%08X, got 0x%08X",
                DW3000_DEVICE_ID, dev_id);
        return -EINVAL;
    }

    LOG_INF("DW3000 initialized successfully");
    return 0;
}

int dw3000_configure(const dw3000_config_t *config)
{
    int ret;

    LOG_INF("Configuring DW3000: Channel=%d, PRF=%d",
            config->channel, config->prf);

    /* Configure channel and PRF */
    uint8_t chan_cfg[4] = {0};
    chan_cfg[0] = config->channel;
    chan_cfg[1] = config->prf;

    ret = dw3000_write_reg(DW3000_REG_SYS_CFG, chan_cfg, sizeof(chan_cfg));
    if (ret < 0) {
        LOG_ERR("Failed to configure channel");
        return ret;
    }

    /* Configure preamble */
    uint8_t preamble_cfg[2] = {0};
    preamble_cfg[0] = config->preamble_length;

    ret = dw3000_write_reg(0x06, preamble_cfg, sizeof(preamble_cfg));
    if (ret < 0) {
        LOG_ERR("Failed to configure preamble");
        return ret;
    }

    LOG_INF("DW3000 configuration complete");
    return 0;
}

int dw3000_rx_enable(uint32_t timeout_ms)
{
    uint8_t cmd[1] = {0x01}; /* RX enable command */

    int ret = dw3000_write_reg(DW3000_REG_SYS_CFG, cmd, sizeof(cmd));
    if (ret < 0) {
        LOG_ERR("Failed to enable RX");
        return ret;
    }

    return 0;
}

bool dw3000_is_frame_ready(void)
{
    uint8_t status[5] = {0};

    int ret = dw3000_read_reg(DW3000_REG_SYS_STATUS, status, sizeof(status));
    if (ret < 0) {
        return false;
    }

    uint32_t status_reg = (status[0] | (status[1] << 8) |
                          (status[2] << 16) | (status[3] << 24));

    return (status_reg & DW3000_STATUS_RXFCG) != 0;
}

int dw3000_read_frame(dw3000_rx_frame_t *frame)
{
    int ret;

    /* Read frame length */
    uint8_t finfo[4] = {0};
    ret = dw3000_read_reg(DW3000_REG_RX_FINFO, finfo, sizeof(finfo));
    if (ret < 0) {
        LOG_ERR("Failed to read frame info");
        return ret;
    }

    frame->length = finfo[0] | ((finfo[1] & 0x03) << 8);
    if (frame->length > sizeof(frame->buffer)) {
        frame->length = sizeof(frame->buffer);
    }

    /* Read frame data */
    ret = dw3000_read_reg(DW3000_REG_RX_BUFFER, frame->buffer, frame->length);
    if (ret < 0) {
        LOG_ERR("Failed to read frame buffer");
        return ret;
    }

    /* Read RX timestamp */
    uint8_t timestamp[5] = {0};
    ret = dw3000_read_reg(DW3000_REG_RX_TIME, timestamp, sizeof(timestamp));
    if (ret < 0) {
        LOG_ERR("Failed to read timestamp");
        return ret;
    }

    frame->timestamp = timestamp[0] | ((uint64_t)timestamp[1] << 8) |
                      ((uint64_t)timestamp[2] << 16) |
                      ((uint64_t)timestamp[3] << 24) |
                      ((uint64_t)timestamp[4] << 32);

    /* Read signal quality metrics */
    uint8_t fqual[8] = {0};
    ret = dw3000_read_reg(DW3000_REG_RX_FQUAL, fqual, sizeof(fqual));
    if (ret < 0) {
        LOG_ERR("Failed to read frame quality");
        return ret;
    }

    /* Calculate RSSI (simplified) */
    uint16_t cir_pwr = fqual[0] | (fqual[1] << 8);
    frame->rssi = 10.0f * log10f((float)cir_pwr) - 115.0f;

    /* Extract first path power index */
    frame->fpp_index = fqual[2] | (fqual[3] << 8);

    /* Calculate first path power level */
    uint16_t fp_ampl = fqual[4] | (fqual[5] << 8);
    frame->fpp_level = 10.0f * log10f((float)fp_ampl);

    /* Frame quality indicator */
    frame->frame_quality = fqual[6];

    /* Clear RX status */
    uint8_t clear_status[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    dw3000_write_reg(DW3000_REG_SYS_STATUS, clear_status, sizeof(clear_status));

    return 0;
}

uint32_t dw3000_get_device_id(void)
{
    uint8_t id[4] = {0};

    int ret = dw3000_read_reg(DW3000_REG_DEV_ID, id, sizeof(id));
    if (ret < 0) {
        return 0;
    }

    return id[0] | (id[1] << 8) | (id[2] << 16) | (id[3] << 24);
}

int dw3000_set_device_address(uint64_t addr)
{
    uint8_t addr_bytes[8];

    for (int i = 0; i < 8; i++) {
        addr_bytes[i] = (addr >> (i * 8)) & 0xFF;
    }

    return dw3000_write_reg(0x03, addr_bytes, sizeof(addr_bytes));
}

int dw3000_reset(void)
{
    LOG_INF("Resetting DW3000");

    /* Soft reset */
    uint8_t reset_cmd[1] = {0xE0};
    int ret = dw3000_write_reg(0x36, reset_cmd, sizeof(reset_cmd));
    if (ret < 0) {
        return ret;
    }

    k_sleep(K_MSEC(10));

    return 0;
}
