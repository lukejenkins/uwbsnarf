# Technical Documentation

## Architecture Overview

The UWB scanner is built on Zephyr RTOS and consists of four main components:

```
┌─────────────────────────────────────────┐
│           Main Application              │
│         (src/main.c)                    │
└─────────────┬───────────────────────────┘
              │
    ┌─────────┴─────────┬─────────────────┐
    │                   │                 │
┌───▼──────────┐  ┌────▼─────────┐  ┌───▼────────────┐
│ UWB Scanner  │  │ UART Output  │  │  Statistics    │
│ (uwb_scanner)│  │ (uart_output)│  │   Thread       │
└──────┬───────┘  └──────────────┘  └────────────────┘
       │
┌──────▼──────────┐
│  DW3000 Driver  │
│ (dw3000_driver) │
└─────────┬───────┘
          │
     ┌────▼─────┐
     │   SPI    │
     │   Bus    │
     └──────────┘
```

## Component Details

### 1. DW3000 Driver (`src/dw3000_driver.c`)

The low-level driver for the DW3000 UWB transceiver chip.

**Key Functions:**
- `dw3000_init()` - Initializes SPI communication and verifies device ID
- `dw3000_configure()` - Sets up channel, PRF, and preamble parameters
- `dw3000_rx_enable()` - Enables receiver mode
- `dw3000_read_frame()` - Reads received frames and extracts metrics

**Hardware Interface:**
- SPI bus at 8 MHz
- GPIO for interrupt, reset, and wakeup pins
- Register-based communication protocol

**Key Registers:**
- `0x00` - Device ID (read-only, should be 0xDECA0302)
- `0x04` - System configuration (channel, PRF)
- `0x10` - RX frame info (length, timestamps)
- `0x11` - RX buffer (frame data)
- `0x12` - RX frame quality (RSSI, SNR, etc.)
- `0x44` - System status (frame ready, errors)

### 2. UWB Scanner (`src/uwb_scanner.c`)

Implements the device scanning logic using the DW3000 driver.

**Operation:**
1. Configures DW3000 for receive mode
2. Continuously listens for UWB frames
3. Parses IEEE 802.15.4 frames to extract device addresses
4. Calculates distance estimates from signal metrics
5. Calls registered callback with device information

**IEEE 802.15.4 Frame Format:**
```
┌─────────┬──────────┬───────────┬────────────┬─────────┬─────┬─────┐
│ FCF (2) │ Seq (1)  │ Dst PAN(2)│ Dst Addr(X)│ Src PAN │ Src │ FCS │
│         │          │           │            │ (2)     │Addr │ (2) │
└─────────┴──────────┴───────────┴────────────┴─────────┴─────┴─────┘
```

**Distance Calculation:**
The scanner uses a simplified path loss model:
```
PL(d) = PL(d0) + 10*n*log10(d/d0)
```
Where:
- `PL(d)` = Path loss at distance d
- `PL(d0)` = Reference path loss at 1m (40 dB)
- `n` = Path loss exponent (2.5 for indoor)
- `d` = Distance to calculate
- `d0` = Reference distance (1m)

**Note:** This is an approximation. For accurate ranging, two-way ranging (TWR) protocol should be implemented.

### 3. UART Output (`src/uart_output.c`)

Formats and outputs device information via UART.

**Output Format:**
All output is in JSON format for easy parsing:

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
  "prf": 64,
  "frame_quality": 200
}
```

**Message Types:**
- `device_found` - A UWB device was detected
- `status` - System status message
- `error` - Error message

### 4. Main Application (`src/main.c`)

Ties everything together and manages application lifecycle.

**Threads:**
- **Main thread** - Initializes subsystems, starts scanner, monitors health
- **Scanner thread** - Runs the UWB scanning loop (priority 5)
- **Statistics thread** - Outputs periodic statistics (priority 7)

## Configuration

### Zephyr Configuration (`prj.conf`)

Key settings:
- `CONFIG_SPI=y` - Enable SPI driver
- `CONFIG_JSON_LIBRARY=y` - Enable JSON formatting
- `CONFIG_FPU=y` - Enable floating-point for calculations
- `CONFIG_MAIN_STACK_SIZE=4096` - Increase stack for UWB processing

### Device Tree (`dwm3001cdk.overlay`)

Defines hardware connections:
- SPI1 bus configuration (pins 14, 16, 17, 20)
- GPIO pins for DW3000 control (IRQ: 19, RST: 24, Wake: 18)
- UART0 for console (TX: 6, RX: 8)

## Performance Characteristics

### Timing
- **Frame processing**: ~5-10ms per frame
- **Scan cycle**: ~60ms (50ms RX window + 10ms processing)
- **Detection latency**: <100ms from transmission to output

### Resource Usage
- **Flash**: ~50-60 KB
- **RAM**: ~8-12 KB
- **CPU**: ~20-30% at full scan rate

### Range and Accuracy
- **Maximum range**: ~50-100m line of sight (hardware dependent)
- **Distance accuracy**: ±10-50cm (using simplified calculation)
- **For better accuracy**: Implement two-way ranging (TWR)

## Extending the Scanner

### Adding Two-Way Ranging (TWR)

For accurate distance measurement, implement TWR:

1. Device A sends ranging request
2. Device B receives and notes timestamp
3. Device B sends ranging response
4. Device A receives and notes timestamp
5. Calculate round-trip time and convert to distance

Distance = (round_trip_time * speed_of_light) / 2

### Multi-Channel Scanning

To scan multiple channels:

```c
const uint8_t channels[] = {5, 9};
for (int i = 0; i < 2; i++) {
    config.channel = channels[i];
    dw3000_configure(&config);
    // Scan for devices...
}
```

### Adding Angle of Arrival (AoA)

The DW3000 supports phase difference measurements for AoA:

1. Use multiple antennas (DWM3001CDK has two antenna ports)
2. Read phase difference from CIA (Channel Impulse Response) registers
3. Calculate angle using: θ = arcsin(λ * Δφ / (2π * d))

Where:
- λ = wavelength
- Δφ = phase difference
- d = antenna separation

## Troubleshooting

### No devices detected

1. Verify another UWB device is transmitting
2. Check channel configuration matches
3. Increase RX timeout
4. Check RSSI threshold

### High error rate

1. Reduce distance between devices
2. Check for interference
3. Adjust PRF and preamble length
4. Verify antenna connection

### Distance inaccurate

1. Implement two-way ranging instead of RSSI-based estimation
2. Calibrate path loss model for your environment
3. Account for antenna delay
4. Use clock offset correction

## References

- DW3000 User Manual (Qorvo)
- IEEE 802.15.4-2020 Standard
- Zephyr RTOS Documentation
- "Decawave's Guide to UWB Ranging"
