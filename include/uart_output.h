/**
 * @file uart_output.h
 * @brief UART output formatting interface
 */

#ifndef UART_OUTPUT_H
#define UART_OUTPUT_H

#include "uwb_scanner.h"

/**
 * @brief Initialize UART output
 *
 * @return 0 on success, negative error code otherwise
 */
int uart_output_init(void);

/**
 * @brief Output device information in JSON format
 *
 * @param info Pointer to device information
 */
void uart_output_device_info(const uwb_device_info_t *info);

/**
 * @brief Output status message
 *
 * @param message Status message string
 */
void uart_output_status(const char *message);

/**
 * @brief Output error message
 *
 * @param error_msg Error message string
 */
void uart_output_error(const char *error_msg);

#endif /* UART_OUTPUT_H */
