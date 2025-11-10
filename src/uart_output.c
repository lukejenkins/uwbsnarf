/**
 * @file uart_output.c
 * @brief UART output formatting implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

#include "uart_output.h"

LOG_MODULE_REGISTER(uart_output, LOG_LEVEL_INF);

/* UART device */
static const struct device *uart_dev;

/* Output buffer */
#define OUTPUT_BUFFER_SIZE 512
static char output_buffer[OUTPUT_BUFFER_SIZE];

/* Mutex for thread-safe output */
static K_MUTEX_DEFINE(output_mutex);

/* Helper function to send string via UART */
static void uart_send_string(const char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        uart_poll_out(uart_dev, str[i]);
    }
}

int uart_output_init(void)
{
    LOG_INF("Initializing UART output");

    /* Get UART device */
    uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    /* Send startup message */
    uart_send_string("\r\n");
    uart_send_string("===========================================\r\n");
    uart_send_string("UWB Device Scanner v1.0\r\n");
    uart_send_string("Qorvo DWM3001CDK\r\n");
    uart_send_string("===========================================\r\n");
    uart_send_string("\r\n");

    LOG_INF("UART output initialized successfully");
    return 0;
}

void uart_output_device_info(const uwb_device_info_t *info)
{
    k_mutex_lock(&output_mutex, K_FOREVER);

    /* Format device information as JSON */
    int len = snprintf(output_buffer, OUTPUT_BUFFER_SIZE,
        "{"
        "\"type\":\"device_found\","
        "\"timestamp_ms\":%u,"
        "\"device_addr\":\"%016llX\","
        "\"distance_cm\":%.2f,"
        "\"rssi_dbm\":%.2f,"
        "\"fpp_index\":%u,"
        "\"fpp_level\":%.2f,"
        "\"channel\":%u,"
        "\"prf\":%u,"
        "\"frame_quality\":%u"
        "}\r\n",
        info->timestamp_ms,
        info->device_addr,
        (double)info->distance_cm,
        (double)info->rssi_dbm,
        info->fpp_index,
        (double)info->fpp_level,
        info->channel,
        info->prf,
        info->frame_quality
    );

    if (len > 0 && len < OUTPUT_BUFFER_SIZE) {
        uart_send_string(output_buffer);
    } else {
        LOG_ERR("Output buffer overflow");
    }

    k_mutex_unlock(&output_mutex);
}

void uart_output_status(const char *message)
{
    k_mutex_lock(&output_mutex, K_FOREVER);

    int len = snprintf(output_buffer, OUTPUT_BUFFER_SIZE,
        "{\"type\":\"status\",\"message\":\"%s\"}\r\n",
        message
    );

    if (len > 0 && len < OUTPUT_BUFFER_SIZE) {
        uart_send_string(output_buffer);
    }

    k_mutex_unlock(&output_mutex);
}

void uart_output_error(const char *error_msg)
{
    k_mutex_lock(&output_mutex, K_FOREVER);

    int len = snprintf(output_buffer, OUTPUT_BUFFER_SIZE,
        "{\"type\":\"error\",\"message\":\"%s\"}\r\n",
        error_msg
    );

    if (len > 0 && len < OUTPUT_BUFFER_SIZE) {
        uart_send_string(output_buffer);
    }

    k_mutex_unlock(&output_mutex);
}
