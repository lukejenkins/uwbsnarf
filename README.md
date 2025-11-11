# UWB Device Scanner for Qorvo DWM3001CDK

This project implements a UWB device scanner that detects nearby UWB devices and outputs comprehensive information via UART.

## Current Status

Broken.

```plaintext
*** Booting Zephyr OS build v4.3.0-rc3-3-gdf8b43d330ed ***
[00:00:00.250,335] <inf> main: USB device enabled
[00:00:00.254,364] <inf> usb_cdc_acm: Device suspended
[17:34:55] STATUS: Initializing UWB scanner...
[00:00:00.627,197] <inf> usb_cdc_acm: Device resumed
[00:00:00.681,640] <inf> usb_cdc_acm: Device configured
[00:00:01.250,427] <inf> main: ==============================================
[00:00:01.250,427] <inf> main:     UWB Device Scanner
[00:00:01.250,427] <inf> main: ==============================================
[00:00:01.250,457] <inf> main: Version:      1.0.0
[00:00:01.250,488] <inf> main: Git Hash:     0789688
[00:00:01.250,518] <inf> main: Build Time:   2025-11-11 00:33:51 UTC
[00:00:01.250,549] <inf> main: Zephyr:       4.3.0-rc3
[00:00:01.250,549] <inf> main: Board:        decawave_dwm3001cdk
 ERROR: UWB scanner initialization failed
[0m
[00:00:01.250,579] <inf> main: ==============================================
[00:00:01.250,579] <inf> uart_output: Initializing UART output
[00:00:01.250,579] <inf> uart_output: USB CDC ACM device ready
[00:00:01.250,610] <inf> uart_output: UART output initialized successfully
[00:00:01.255,920] <inf> uwb_scanner: Initializing UWB scanner
[00:00:01.255,950] <inf> dw3000: Initializing DW3000
[00:00:01.255,950] <inf> dw3000: Performing hardware reset
[00:00:01.266,082] <inf> dw3000: Device ID (attempt 1): 0xFFFFFFFF
[00:00:01.276,306] <inf> dw3000: Device ID (attempt 2): 0xFFFFFFFF
[00:00:01.286,529] <inf> dw3000: Device ID (attempt 3): 0xFFFFFFFF
[00:00:01.296,752] <inf> dw3000: Device ID (attempt 4): 0xFFFFFFFF
[00:00:01.306,976] <inf> dw3000: Device ID (attempt 5): 0xFFFFFFFF
[00:00:01.317,077] <err> dw3000: Device ID reads as 0xFFFFFFFF - chip not responding or not powered
[00:00:01.317,077] <err> uwb_scanner: Failed to initialize DW3000: -5
[00:00:01.317,077] <err> main: Failed to initialize UWB scanner: -5
```

## Features

- Continuous UWB device scanning
- Detailed device information output including:
  - Device address (EUI-64)
  - Signal strength (RSSI)
  - Estimated distance
  - Timestamp
  - Frame quality indicators
  - Channel and PRF information
- UART output (115200 baud, 8N1)
- JSON-formatted output for easy parsing

## Hardware Requirements

- Qorvo DWM3001CDK development board
- USB cable for power and UART communication
- At least one other UWB device for testing

## Building

This project uses Zephyr RTOS. To build:

```bash
# Set up Zephyr environment (first time only)
west init -m https://github.com/zephyrproject-rtos/zephyr --mr main uwb-workspace
cd uwb-workspace
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt

# Copy this project to uwb-workspace/uwbsnarf
# Then build:
west build -b dwm3001cdk uwbsnarf
west flash
```

## Usage

1. Flash the firmware to your DWM3001CDK
2. Connect via serial terminal (115200 baud): `screen /dev/ttyACM0 115200`
3. The device will automatically start scanning and outputting detected devices

## Output Format

The scanner outputs JSON lines with device information:

```json
{
  "type": "device_found",
  "timestamp_ms": 12345,
  "device_addr": "0123456789ABCDEF",
  "distance_cm": 123.45,
  "rssi_dbm": -65.2,
  "fpp_index": 245,
  "fpp_level": 12.3,
  "channel": 5,
  "prf": 64
}
```

## Architecture

- `src/main.c` - Main application and initialization
- `src/uwb_scanner.c` - UWB scanning implementation
- `src/dw3000_driver.c` - DW3000 hardware driver
- `src/uart_output.c` - UART formatting and output

## AI Disclosure

**Here there be robots!** I *think* they are friendly, but they might just be very good at pretending. You might be a fool if you use this project for anything other than as an example of how silly it can be to use AI to code with.

> This project was developed with the assistance of language models from OpenAI and Anthropic, which provided suggestions and code snippets to enhance the functionality and efficiency of the tools. The models were used to generate code, documentation, distraction, moral support, moral turpitude, and explanations for various components of the project.
