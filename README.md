# UWB Device Scanner for Qorvo DWM3001CDK

This project implements a UWB device scanner that detects nearby UWB devices and outputs comprehensive information via UART.

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

## License

MIT
