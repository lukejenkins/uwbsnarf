# Setup Guide for UWB Scanner

This guide will help you set up the development environment and build the UWB scanner firmware.

## Prerequisites

### Hardware
- Qorvo DWM3001CDK development board
- USB cable (USB-A to Micro-USB)
- Computer running Linux, macOS, or Windows (WSL2)

### Software
- Python 3.8 or newer
- Git
- CMake 3.20 or newer
- Device Tree Compiler (dtc)
- ARM GCC Toolchain

## Step 1: Install Zephyr Dependencies

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

### macOS
```bash
brew install cmake ninja gperf python3 ccache qemu dtc wget libmagic
```

## Step 2: Set Up Zephyr

```bash
# Install West (Zephyr's meta-tool)
pip3 install --user -U west

# Create workspace and clone Zephyr
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.5.0 ~/zephyr-workspace
cd ~/zephyr-workspace
west update

# Export Zephyr CMake package
west zephyr-export

# Install Python dependencies
pip3 install --user -r ~/zephyr-workspace/zephyr/scripts/requirements.txt
```

## Step 3: Install ARM Toolchain

### Linux
```bash
cd ~
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
```

Add to your `~/.bashrc`:
```bash
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=~/gcc-arm-none-eabi-10.3-2021.10
```

### macOS
```bash
brew install --cask gcc-arm-embedded
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr/local
```

## Step 4: Clone and Build Project

```bash
# Copy the uwbsnarf project to Zephyr workspace
cp -r /path/to/uwbsnarf ~/zephyr-workspace/

# Source Zephyr environment
cd ~/zephyr-workspace
source zephyr/zephyr-env.sh

# Build the project
cd uwbsnarf
./scripts/build.sh
```

## Step 5: Flash the Firmware

Connect your DWM3001CDK via USB and flash:

```bash
./scripts/flash.sh
```

## Step 6: Monitor Output

```bash
# Install pyserial if not already installed
pip3 install pyserial

# Run the monitor
python3 scripts/monitor.py /dev/ttyACM0
```

On macOS, the device might be at `/dev/tty.usbmodem*`. On Windows (WSL2), it might be at `/dev/ttyS*`.

## Troubleshooting

### Device not detected
- Check USB connection
- Verify device appears in `lsusb` output
- Check permissions: `sudo usermod -a -G dialout $USER` (then log out and back in)

### Build errors
- Ensure Zephyr environment is sourced: `source ~/zephyr-workspace/zephyr/zephyr-env.sh`
- Clean build: `rm -rf build && ./scripts/build.sh`

### No output on serial
- Check baud rate is 115200
- Verify correct serial port
- Try resetting the device

### DW3000 initialization fails
- Check SPI connections
- Verify device tree overlay is correct
- Check power supply (should be USB powered)

## Next Steps

Once the scanner is running, you should see JSON output with device information. You can:

1. Parse the JSON output in your own applications
2. Log data to a file: `python3 scripts/monitor.py /dev/ttyACM0 -o log.txt`
3. Modify the firmware to change scanning parameters
4. Integrate with additional sensors or displays

## Additional Resources

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [DWM3001CDK User Manual](https://www.qorvo.com/products/p/DWM3001CDK)
- [DW3000 Datasheet](https://www.qorvo.com/products/d/da008286)
- [IEEE 802.15.4 Standard](https://standards.ieee.org/standard/802_15_4-2020.html)
