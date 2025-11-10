/**
 * @file uwb_scanner.c
 * @brief UWB device scanner implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

#include "uwb_scanner.h"
#include "dw3000_driver.h"

LOG_MODULE_REGISTER(uwb_scanner, LOG_LEVEL_INF);

/* Scanner state */
static bool scanner_active = false;
static uwb_device_callback_t device_callback = NULL;

/* Scanner thread */
#define SCANNER_STACK_SIZE 2048
#define SCANNER_PRIORITY 5

static K_THREAD_STACK_DEFINE(scanner_stack, SCANNER_STACK_SIZE);
static struct k_thread scanner_thread;

/* IEEE 802.15.4 frame types */
#define IEEE154_FRAME_TYPE_BEACON 0x00
#define IEEE154_FRAME_TYPE_DATA   0x01
#define IEEE154_FRAME_TYPE_ACK    0x02
#define IEEE154_FRAME_TYPE_MAC    0x03

/* Frame control field parsing */
typedef struct {
    uint8_t frame_type;
    bool security_enabled;
    bool frame_pending;
    bool ack_request;
    bool pan_id_compress;
    uint8_t dest_addr_mode;
    uint8_t src_addr_mode;
} ieee154_fcf_t;

/* Parse IEEE 802.15.4 frame control field */
static void parse_frame_control(uint16_t fcf, ieee154_fcf_t *parsed)
{
    parsed->frame_type = fcf & 0x07;
    parsed->security_enabled = (fcf & 0x08) != 0;
    parsed->frame_pending = (fcf & 0x10) != 0;
    parsed->ack_request = (fcf & 0x20) != 0;
    parsed->pan_id_compress = (fcf & 0x40) != 0;
    parsed->dest_addr_mode = (fcf >> 10) & 0x03;
    parsed->src_addr_mode = (fcf >> 14) & 0x03;
}

/* Extract device address from frame */
static uint64_t extract_device_address(const uint8_t *frame, uint16_t length,
                                      const ieee154_fcf_t *fcf)
{
    uint64_t addr = 0;
    int offset = 3; /* Skip FCF and sequence number */

    /* Skip destination PAN ID if present */
    if (fcf->dest_addr_mode != 0) {
        offset += 2;
    }

    /* Skip destination address */
    if (fcf->dest_addr_mode == 2) {
        offset += 2; /* Short address */
    } else if (fcf->dest_addr_mode == 3) {
        offset += 8; /* Extended address */
    }

    /* Skip source PAN ID if present and not compressed */
    if (fcf->src_addr_mode != 0 && !fcf->pan_id_compress) {
        offset += 2;
    }

    /* Extract source address */
    if (fcf->src_addr_mode == 3 && offset + 8 <= length) {
        /* Extended 64-bit address */
        for (int i = 0; i < 8; i++) {
            addr |= ((uint64_t)frame[offset + i]) << (i * 8);
        }
    } else if (fcf->src_addr_mode == 2 && offset + 2 <= length) {
        /* Short 16-bit address */
        addr = frame[offset] | ((uint64_t)frame[offset + 1] << 8);
    }

    return addr;
}

/* Calculate distance from first path power metrics */
static float calculate_distance(uint16_t fpp_index, float fpp_level, float rssi)
{
    /* Simplified distance calculation based on two-way ranging time */
    /* This is an approximation - actual ranging requires two-way exchange */

    /* Convert RSSI to distance using path loss model */
    /* Path loss: PL(d) = PL(d0) + 10*n*log10(d/d0) */
    /* Where n is path loss exponent (typically 2-4 for indoor) */

    const float tx_power = 0.0f;  /* dBm */
    const float pl_d0 = 40.0f;     /* Path loss at 1m reference */
    const float path_loss_exp = 2.5f;  /* Indoor environment */

    float path_loss = tx_power - rssi;
    float distance_m = powf(10.0f, (path_loss - pl_d0) / (10.0f * path_loss_exp));

    /* Convert to centimeters */
    return distance_m * 100.0f;
}

/* Scanner thread function */
static void scanner_thread_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    LOG_INF("Scanner thread started");

    dw3000_rx_frame_t rx_frame;
    uwb_device_info_t device_info;

    while (scanner_active) {
        /* Enable receiver */
        int ret = dw3000_rx_enable(100); /* 100ms timeout */
        if (ret < 0) {
            LOG_ERR("Failed to enable RX: %d", ret);
            k_sleep(K_MSEC(100));
            continue;
        }

        /* Wait for frame or timeout */
        k_sleep(K_MSEC(50));

        /* Check if frame is ready */
        if (dw3000_is_frame_ready()) {
            /* Read the frame */
            ret = dw3000_read_frame(&rx_frame);
            if (ret < 0) {
                LOG_ERR("Failed to read frame: %d", ret);
                continue;
            }

            LOG_DBG("Frame received: length=%d, RSSI=%.2f dBm",
                   rx_frame.length, rx_frame.rssi);

            /* Parse frame to extract device information */
            if (rx_frame.length >= 3) {
                uint16_t fcf = rx_frame.buffer[0] | (rx_frame.buffer[1] << 8);
                ieee154_fcf_t fcf_parsed;
                parse_frame_control(fcf, &fcf_parsed);

                /* Extract device address */
                device_info.device_addr = extract_device_address(
                    rx_frame.buffer, rx_frame.length, &fcf_parsed);

                /* Only report valid addresses */
                if (device_info.device_addr != 0) {
                    /* Fill in device information */
                    device_info.timestamp_ms = k_uptime_get_32();
                    device_info.rssi_dbm = rx_frame.rssi;
                    device_info.fpp_index = rx_frame.fpp_index;
                    device_info.fpp_level = rx_frame.fpp_level;
                    device_info.frame_quality = rx_frame.frame_quality;
                    device_info.channel = 5; /* From configuration */
                    device_info.prf = 64;    /* From configuration */

                    /* Calculate distance */
                    device_info.distance_cm = calculate_distance(
                        rx_frame.fpp_index, rx_frame.fpp_level, rx_frame.rssi);

                    LOG_INF("Device detected: addr=0x%016llX, RSSI=%.2f dBm, dist=%.2f cm",
                           device_info.device_addr, device_info.rssi_dbm,
                           device_info.distance_cm);

                    /* Call callback if registered */
                    if (device_callback != NULL) {
                        device_callback(&device_info);
                    }
                }
            }
        }

        /* Small delay between scans */
        k_sleep(K_MSEC(10));
    }

    LOG_INF("Scanner thread stopped");
}

int uwb_scanner_init(uwb_device_callback_t callback)
{
    LOG_INF("Initializing UWB scanner");

    /* Store callback */
    device_callback = callback;

    /* Initialize DW3000 */
    int ret = dw3000_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize DW3000: %d", ret);
        return ret;
    }

    /* Configure DW3000 for scanning */
    dw3000_config_t config = {
        .channel = DW3000_CHANNEL_5,
        .prf = DW3000_PRF_64M,
        .preamble_length = DW3000_PLEN_128,
        .pac_size = 8,
        .tx_preamble_code = 9,
        .rx_preamble_code = 9,
    };

    ret = dw3000_configure(&config);
    if (ret < 0) {
        LOG_ERR("Failed to configure DW3000: %d", ret);
        return ret;
    }

    LOG_INF("UWB scanner initialized successfully");
    return 0;
}

int uwb_scanner_start(void)
{
    if (scanner_active) {
        LOG_WRN("Scanner already active");
        return -EALREADY;
    }

    LOG_INF("Starting UWB scanner");
    scanner_active = true;

    /* Create scanner thread */
    k_thread_create(&scanner_thread, scanner_stack, SCANNER_STACK_SIZE,
                   scanner_thread_fn, NULL, NULL, NULL,
                   SCANNER_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(&scanner_thread, "uwb_scanner");

    return 0;
}

int uwb_scanner_stop(void)
{
    if (!scanner_active) {
        LOG_WRN("Scanner not active");
        return -EALREADY;
    }

    LOG_INF("Stopping UWB scanner");
    scanner_active = false;

    /* Wait for thread to finish */
    k_thread_join(&scanner_thread, K_SECONDS(5));

    return 0;
}

bool uwb_scanner_is_active(void)
{
    return scanner_active;
}
