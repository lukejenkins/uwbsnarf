# UWB Scanner - AI Agent Instructions

## Project Overview
This is a **Zephyr RTOS embedded firmware** project for the Qorvo DWM3001CDK board that implements a UWB device scanner. It detects nearby UWB devices and outputs JSON-formatted device information via UART/USB CDC-ACM.

**Target Hardware:** Nordic nRF52840 SoC with DW3000 UWB transceiver
**Build System:** Zephyr RTOS + West + CMake
**Language:** C (Zephyr kernel APIs, not standard libc)

## Architecture (4 Main Components)

```
main.c → uwb_scanner.c → dw3000_driver.c → SPI Bus
  ↓
uart_output.c (JSON formatting)
```

1. **`src/dw3000_driver.c`** - Low-level SPI driver for DW3000 UWB chip
   - Register-based communication (see `DW3000_REG_*` defines in `include/dw3000_driver.h`)
   - Key: Device expects SPI Mode 0, 8MHz max frequency
   - Hardware reset via GPIO pin 24 (active-low)

2. **`src/uwb_scanner.c`** - IEEE 802.15.4 frame parser & device tracker
   - Parses frame control field (FCF) to extract 64-bit device addresses
   - Distance calculation uses RSSI-based path loss model (NOT accurate ranging)
   - For real ranging, implement Two-Way Ranging (TWR) protocol

3. **`src/uart_output.c`** - JSON output formatter
   - All output is JSON lines: `{"type": "device_found", "device_addr": "...", ...}`
   - Message types: `device_found`, `status`, `error`

4. **`src/main.c`** - Application orchestration
   - Creates 2 threads: scanner thread (prio 5) and statistics thread (prio 7)
   - USB CDC-ACM auto-enumeration with 1s delay on startup
   - Scanner health monitoring with auto-restart

## Critical Build & Flash Workflow

### Build Commands (ALWAYS use these scripts, NOT direct `west build`)
```bash
./scripts/build.sh   # Auto-detects Zephyr workspace, SDK, activates .venv
./scripts/flash.sh   # Flashes firmware via west flash
python3 scripts/monitor.py /dev/ttyACM0  # Serial monitor with JSON parsing
```

**Why scripts?** They auto-detect:
- Zephyr workspace: `~/zephyrproject` or `~/zephyr-workspace`
- Zephyr SDK: `~/zephyrproject/zephyr-sdk-0.17.0` or `~/zephyr-sdk-0.17.0`
- Python venv: `.venv` in Zephyr workspace
- Set `ZEPHYR_BASE` and `ZEPHYR_SDK_INSTALL_DIR` automatically

Build artifacts go to `<zephyr-workspace>/build/`, NOT `./build/` (the local build/ is old/cached).

### Device Tree Overlay Critical Details
File: `decawave_dwm3001cdk.overlay`
- DW3000 on SPI3: SCK=P0.16, MOSI=P0.20, MISO=P0.21, CS=P0.17
- GPIO pins: RESET=24, WAKEUP=18, IRQ=19
- USB CDC-ACM is primary console (not physical UART)

**If SPI fails:** Check CS GPIO in overlay matches `spi_cfg.cs` in `dw3000_driver.c`

## Project-Specific Conventions

### Zephyr API Patterns (NOT standard C)
- Threads: `k_thread_create()`, `K_THREAD_STACK_DEFINE()`, `k_sleep(K_MSEC(x))`
- Logging: `LOG_MODULE_REGISTER()`, `LOG_INF/ERR/WRN()` (not printf)
- Device Tree: `DEVICE_DT_GET(DT_NODELABEL(name))`, `device_is_ready()`
- Timers: `k_uptime_get_32()` for milliseconds since boot

### Error Handling Pattern
All functions return `int`: 0=success, negative=error (Zephyr convention).
Example from `dw3000_init()`:
```c
if (!device_is_ready(spi_dev)) {
    LOG_ERR("SPI device not ready");
    return -ENODEV;  // Use Zephyr errno codes
}
```

### Hardware-Specific Constants
- **Device ID verification:** DW3000 register 0x00 must read `0xDECA0302`
- **SPI timing:** 2μs CS delay required (`spi_cfg.cs.delay = 2`)
- **Channel config:** Only channels 5 and 9 supported (see `DW3000_CHANNEL_*`)

### Distance Calculation Limitation
Current implementation uses **RSSI-based estimation** (±10-50cm accuracy). This is documented in `TECHNICAL.md` as a simplification. For centimeter-accurate ranging, you must implement TWR protocol (see `TECHNICAL.md` "Adding Two-Way Ranging" section).

## Configuration Files

### `prj.conf` Key Settings
```conf
CONFIG_FPU=y                       # Required for float distance calculations
CONFIG_MAIN_STACK_SIZE=8192        # Large stack for UWB frame processing
CONFIG_JSON_LIBRARY=y              # For uart_output.c JSON formatting
CONFIG_USB_CDC_ACM=y               # Primary UART interface
CONFIG_SPEED_OPTIMIZATIONS=y       # Timing-critical for UWB
```

### CMake Version Injection
`CMakeLists.txt` injects git commit hash and build timestamp:
```cmake
target_compile_definitions(app PRIVATE
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    BUILD_TIMESTAMP="${BUILD_TIMESTAMP}"
)
```
These appear in startup logs via `src/main.c`.

## Debugging Workflows

### Check Scanner Health
Look for in `main.c` loop:
```c
if (!uwb_scanner_is_active()) {
    LOG_WRN("Scanner stopped unexpectedly");
    // Auto-restart logic here
}
```

### Common Issues
1. **"SPI device not ready"** → Check Device Tree overlay SPI3 node status
2. **"Device ID mismatch"** → DW3000 not responding, check reset GPIO/power
3. **No devices detected** → Another UWB device must be transmitting on same channel
4. **JSON parse errors in monitor.py** → Check UART baud rate (115200) and line endings

### Serial Port Names
- Linux: `/dev/ttyACM0`
- macOS: `/dev/tty.usbmodem*` (use tab completion)

## When Extending the Code

### Adding New DW3000 Registers
1. Define in `include/dw3000_driver.h`: `#define DW3000_REG_NEWREG 0xXX`
2. Use `dw3000_read_reg()` / `dw3000_write_reg()` (handles SPI framing)
3. See DW3000 User Manual for register layout

### Adding New JSON Output Fields
1. Extend `uwb_device_info_t` struct in `include/uwb_scanner.h`
2. Update `uart_output_device_info()` in `src/uart_output.c`
3. Update `monitor.py` parser for display

### Thread Priority Rules
Lower number = higher priority in Zephyr. Current allocation:
- Scanner: 5 (time-critical UWB processing)
- Statistics: 7 (background reporting)

## External Dependencies
- **Zephyr v3.5.0** (or compatible) - MUST be in `~/zephyrproject/zephyr`
- **Zephyr SDK 0.17.0** - Includes ARM GCC toolchain
- **Python packages:** `pyserial` for `monitor.py` (see `requirements.txt`)

## References for Deep Dives
- `TECHNICAL.md` - Architecture diagrams, register maps, distance calculations
- `SETUP.md` - Complete environment setup guide
- DW3000 User Manual - Register definitions (not in repo)
- IEEE 802.15.4-2020 - Frame format specification
