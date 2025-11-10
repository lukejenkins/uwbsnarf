/**
 * @file main.c
 * @brief UWB Device Scanner main application
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include "uwb_scanner.h"
#include "uart_output.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Statistics */
static uint32_t devices_detected = 0;
static uint32_t scan_start_time = 0;

/* Device discovery callback */
static void on_device_found(const uwb_device_info_t *info)
{
    devices_detected++;

    /* Output device information via UART */
    uart_output_device_info(info);

    /* Log summary */
    LOG_INF("Device #%u: addr=0x%016llX, dist=%.2f cm, RSSI=%.2f dBm",
           devices_detected,
           info->device_addr,
           (double)info->distance_cm,
           (double)info->rssi_dbm);
}

/* Statistics thread */
#define STATS_THREAD_STACK_SIZE 1024
#define STATS_THREAD_PRIORITY 7

static K_THREAD_STACK_DEFINE(stats_stack, STATS_THREAD_STACK_SIZE);
static struct k_thread stats_thread;
static bool stats_active = true;

static void stats_thread_fn(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (stats_active) {
        /* Wait 10 seconds */
        k_sleep(K_SECONDS(10));

        /* Calculate and output statistics */
        uint32_t uptime = k_uptime_get_32();
        uint32_t scan_duration = (uptime - scan_start_time) / 1000;

        char status_msg[128];
        snprintf(status_msg, sizeof(status_msg),
                "Uptime: %u s, Devices detected: %u, Scan duration: %u s",
                uptime / 1000, devices_detected, scan_duration);

        uart_output_status(status_msg);

        LOG_INF("Statistics: %s", status_msg);
    }
}

int main(void)
{
    int ret;

    printk("\n");
    printk("===========================================\n");
    printk("UWB Device Scanner\n");
    printk("Qorvo DWM3001CDK\n");
    printk("===========================================\n");
    printk("\n");

    LOG_INF("Starting UWB Device Scanner");

    /* Initialize UART output */
    ret = uart_output_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize UART output: %d", ret);
        uart_output_error("UART initialization failed");
        return ret;
    }

    uart_output_status("Initializing UWB scanner...");

    /* Initialize UWB scanner */
    ret = uwb_scanner_init(on_device_found);
    if (ret < 0) {
        LOG_ERR("Failed to initialize UWB scanner: %d", ret);
        uart_output_error("UWB scanner initialization failed");
        return ret;
    }

    uart_output_status("UWB scanner initialized");

    /* Start scanning */
    scan_start_time = k_uptime_get_32();
    ret = uwb_scanner_start();
    if (ret < 0) {
        LOG_ERR("Failed to start UWB scanner: %d", ret);
        uart_output_error("Failed to start scanner");
        return ret;
    }

    uart_output_status("Scanning started");
    LOG_INF("Scanner started successfully");

    /* Start statistics thread */
    k_thread_create(&stats_thread, stats_stack, STATS_THREAD_STACK_SIZE,
                   stats_thread_fn, NULL, NULL, NULL,
                   STATS_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&stats_thread, "statistics");

    /* Main loop - just keep running */
    while (1) {
        k_sleep(K_SECONDS(1));

        /* Check if scanner is still active */
        if (!uwb_scanner_is_active()) {
            LOG_WRN("Scanner stopped unexpectedly");
            uart_output_error("Scanner stopped");

            /* Try to restart */
            k_sleep(K_SECONDS(1));
            ret = uwb_scanner_start();
            if (ret < 0) {
                LOG_ERR("Failed to restart scanner: %d", ret);
            } else {
                uart_output_status("Scanner restarted");
            }
        }
    }

    return 0;
}
