/**
 * @file dw3000_driver.c
 * @brief DW3000 UWB transceiver driver implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <math.h>

#include "dw3000_driver.h"

LOG_MODULE_REGISTER(dw3000, LOG_LEVEL_INF);

/* Device Tree Node */
#define DW3000_NODE DT_NODELABEL(dw3000)

/* SPI Configuration */
#define DW3000_SPI_FREQ 8000000
#define DW3000_SPI_FREQ_SLOW 2000000  /* Slower speed for init */
#define DW3000_SPI_MODE (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

/* SPI Commands */
#define DW3000_SPI_WRITE 0x80
#define DW3000_SPI_READ  0x00

/* Device constants */
#define DW3000_DEVICE_ID 0xDECA0302

/* GPIO Pins for DWM3001CDK */
#define DW3000_RESET_PIN  24
#define DW3000_WAKEUP_PIN 18
#define DW3000_IRQ_PIN    19

/* Static variables */
static const struct device *spi_dev;
static struct spi_config spi_cfg;
static const struct device *gpio_dev;

/* Helper function to perform SPI transaction */
static int dw3000_spi_transfer(uint16_t reg, uint8_t *data, uint16_t len, bool write)
{
    uint8_t header[3];
    uint8_t header_rx[3];
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
        {.buf = header, .len = (size_t)header_len},
        {.buf = write ? data : NULL, .len = write ? len : 0}
    };

    struct spi_buf rx_bufs[] = {
        {.buf = header_rx, .len = (size_t)header_len},
        {.buf = write ? NULL : data, .len = write ? 0 : len}
    };

    struct spi_buf_set tx = {.buffers = tx_bufs, .count = 2U};
    struct spi_buf_set rx = {.buffers = rx_bufs, .count = 2U};

    int ret = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
    if (ret < 0) {
        LOG_ERR("SPI transfer failed: %d (reg=0x%04X, len=%d, write=%d)", 
                ret, reg, len, write);
        return ret;
    }
    
    /* Debug: Print first few bytes of read data */
    if (!write && len > 0) {
        LOG_DBG("SPI read reg 0x%04X: 0x%02X 0x%02X 0x%02X 0x%02X", 
                reg, 
                len > 0 ? data[0] : 0,
                len > 1 ? data[1] : 0,
                len > 2 ? data[2] : 0,
                len > 3 ? data[3] : 0);
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
    int ret;

    LOG_INF("Initializing DW3000");

    /* Get GPIO device */
    gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }

    /* Get SPI device */
    spi_dev = DEVICE_DT_GET(DT_BUS(DW3000_NODE));
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI device not ready");
        return -ENODEV;
    }
    LOG_DBG("SPI device ready: %s", spi_dev->name);

    /* Configure SPI - DW3000 requires SPI Mode 0 (CPOL=0, CPHA=0) */
    /* Start with slower speed for initialization */
    spi_cfg.frequency = DW3000_SPI_FREQ_SLOW;
    spi_cfg.operation = DW3000_SPI_MODE;
    spi_cfg.slave = 0;

    /* Configure CS control from device tree */
    spi_cfg.cs = (struct spi_cs_control) {
        .gpio = SPI_CS_GPIOS_DT_SPEC_GET(DW3000_NODE),
        .delay = 2,  /* 2us delay after CS assertion */
    };
    
    LOG_DBG("SPI config: freq=%d Hz, mode=0x%08X", spi_cfg.frequency, spi_cfg.operation);

    /* Configure reset GPIO (output, initially high/inactive since active-low) */
    ret = gpio_pin_configure(gpio_dev, DW3000_RESET_PIN, GPIO_OUTPUT_HIGH);
    if (ret < 0) {
        LOG_ERR("Failed to configure reset GPIO: %d", ret);
        return ret;
    }

    /* Configure wakeup GPIO (keep high to prevent sleep) */
    ret = gpio_pin_configure(gpio_dev, DW3000_WAKEUP_PIN, GPIO_OUTPUT_HIGH);
    if (ret < 0) {
        LOG_WRN("Failed to configure wakeup GPIO: %d", ret);
    }

    /* Perform hardware reset sequence */
    LOG_INF("Performing hardware reset");

    /* DW3000 wakeup sequence: 
     * The chip might be in deep sleep. We need to wake it first before reset.
     * Wakeup requires pulling WAKEUP low briefly, then high.
     */
    
    /* First, try to wake the chip from deep sleep */
    LOG_DBG("Waking chip from potential deep sleep");
    gpio_pin_set(gpio_dev, DW3000_WAKEUP_PIN, 0);  /* Pull WAKEUP low */
    k_sleep(K_USEC(500));  /* 500us low pulse */
    gpio_pin_set(gpio_dev, DW3000_WAKEUP_PIN, 1);  /* Pull WAKEUP high */
    k_sleep(K_MSEC(2));  /* Wait for wakeup */

    /* Now perform the hardware reset */
    LOG_DBG("Asserting reset (low)");
    gpio_pin_set(gpio_dev, DW3000_RESET_PIN, 0);
    k_sleep(K_MSEC(2));  /* Hold reset for 2ms minimum */

    /* Deassert reset (pull high) */
    LOG_DBG("Deasserting reset (high)");
    gpio_pin_set(gpio_dev, DW3000_RESET_PIN, 1);
    
    /* Keep WAKEUP high to prevent chip from going back to sleep */
    gpio_pin_set(gpio_dev, DW3000_WAKEUP_PIN, 1);
    
    /* Wait for chip to stabilize after reset - DW3000 datasheet specifies 5ms */
    k_sleep(K_MSEC(5));
    
    LOG_DBG("Attempting to read device ID");

    /* Try a simple SPI loopback test first by reading a known register */
    uint8_t test_buf[4] = {0};
    int ret_test = dw3000_read_reg(DW3000_REG_DEV_ID, test_buf, 4);
    LOG_DBG("Initial SPI test: ret=%d, data=[0x%02X 0x%02X 0x%02X 0x%02X]",
            ret_test, test_buf[0], test_buf[1], test_buf[2], test_buf[3]);

    /* Read and verify device ID - try multiple times */
    uint32_t dev_id = 0;
    int attempts = 0;
    for (attempts = 0; attempts < 5; attempts++) {
        dev_id = dw3000_get_device_id();
        LOG_INF("Device ID (attempt %d): 0x%08X", attempts + 1, dev_id);
        
        /* Check if we got a valid response (not 0x00000000 or 0xFFFFFFFF) */
        if (dev_id != 0x00000000 && dev_id != 0xFFFFFFFF) {
            break;
        }
        
        /* Wait a bit before retrying */
        k_sleep(K_MSEC(10));
    }
    
    /* If we still have all 1s or all 0s, there's a communication problem */
    if (dev_id == 0x00000000) {
        LOG_ERR("Device ID reads as 0x00000000 - possible SPI connection issue");
        return -EIO;
    } else if (dev_id == 0xFFFFFFFF) {
        LOG_ERR("Device ID reads as 0xFFFFFFFF - chip not responding or not powered");
        return -EIO;
    }

    if ((dev_id & 0xFFFFFF00) != (DW3000_DEVICE_ID & 0xFFFFFF00)) {
        LOG_ERR("Invalid device ID: expected 0x%08X, got 0x%08X",
                DW3000_DEVICE_ID, dev_id);
        return -EINVAL;
    }

    LOG_INF("DW3000 detected successfully, switching to full speed SPI");

    /* Now that we confirmed the chip is working, switch to full speed */
    spi_cfg.frequency = DW3000_SPI_FREQ;
    
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
