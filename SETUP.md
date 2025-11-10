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
# Note: You can use either ~/zephyrproject or ~/zephyr-workspace
# The build scripts will auto-detect both locations
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.5.0 ~/zephyrproject
cd ~/zephyrproject
west update

# Export Zephyr CMake package
west zephyr-export

# Set up Python virtual environment (recommended)
python3 -m venv .venv
source .venv/bin/activate

# Install Python dependencies
pip3 install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

**Important Environment Variables:**

The build scripts will automatically detect and set these, but you can also set them manually:

- **ZEPHYR_BASE**: Path to Zephyr repository (auto-detected from workspace)
- **ZEPHYR_SDK_INSTALL_DIR**: Path to Zephyr SDK installation
  - Auto-detected locations: `~/zephyrproject/zephyr-sdk-0.17.0` or `~/zephyr-sdk-0.17.0`
- **Python .venv**: If present in the Zephyr workspace, will be auto-activated

## Step 3: Install Zephyr SDK (Recommended)

The Zephyr SDK includes the ARM toolchain and other tools needed for building. This is the recommended approach.

### Download and Install Zephyr SDK

```bash
# Download Zephyr SDK 0.17.0 (adjust version as needed)
cd ~/zephyrproject
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_macos-x86_64.tar.xz  # macOS Intel
# Or for macOS ARM: zephyr-sdk-0.17.0_macos-aarch64.tar.xz
# Or for Linux: zephyr-sdk-0.17.0_linux-x86_64.tar.xz

# Extract
tar xvf zephyr-sdk-0.17.0_*.tar.xz

# Run the SDK setup script
cd zephyr-sdk-0.17.0
./setup.sh

# The build scripts will auto-detect the SDK at:
# - ~/zephyrproject/zephyr-sdk-0.17.0
# - ~/zephyr-sdk-0.17.0
```

**Note:** If you install the SDK to a different location, set the environment variable:
```bash
export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk-0.17.0
```

### Alternative: ARM GCC Toolchain Only

If you prefer not to use the Zephyr SDK, you can install just the ARM toolchain:

#### Linux
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

#### macOS
```bash
brew install --cask gcc-arm-embedded
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr/local
```

## Step 4: Clone and Build Project

```bash
# Clone the uwbsnarf project (if you haven't already)
git clone <repository-url> ~/uwbsnarf
cd ~/uwbsnarf

# Build the project (no need to manually source environment!)
./scripts/build.sh
```

The build script will automatically:
- Detect your Zephyr workspace at `~/zephyrproject` or `~/zephyr-workspace`
- Activate the Python virtual environment if present
- Source the Zephyr environment
- Auto-detect the Zephyr SDK at common locations
- Build the firmware for the DWM3001CDK board

**Manual Environment Setup (Optional):**

If you prefer to set up the environment manually or the auto-detection fails:

```bash
# Navigate to Zephyr workspace
cd ~/zephyrproject

# Activate Python virtual environment
source .venv/bin/activate

# Source Zephyr environment
source zephyr/zephyr-env.sh

# Set SDK location if not auto-detected
export ZEPHYR_SDK_INSTALL_DIR=~/zephyrproject/zephyr-sdk-0.17.0

# Then build from your project directory
cd ~/uwbsnarf
./scripts/build.sh
```

## Step 5: Flash the Firmware

Connect your DWM3001CDK via USB and flash:

```bash
./scripts/flash.sh
```

The flash script will automatically handle environment setup, just like the build script.

## Step 6: Monitor Output

```bash
# Install pyserial if not already installed
pip3 install pyserial

# Run the monitor
python3 scripts/monitor.py /dev/ttyACM0
```

On macOS, the device might be at `/dev/tty.usbmodem*`. On Windows (WSL2), it might be at `/dev/ttyS*`.

## Troubleshooting

### Build script cannot find Zephyr workspace
If the build script fails to auto-detect your Zephyr installation:

1. Ensure Zephyr is installed at `~/zephyrproject` or `~/zephyr-workspace`
2. Or manually set the environment:
   ```bash
   export ZEPHYR_BASE=~/path/to/zephyr
   ./scripts/build.sh
   ```

### ZEPHYR_SDK_INSTALL_DIR not found
If you see warnings about the SDK not being found:

1. Install the Zephyr SDK to a standard location:
   - `~/zephyrproject/zephyr-sdk-0.17.0`
   - `~/zephyr-sdk-0.17.0`
2. Or set the environment variable:
   ```bash
   export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk-0.17.0
   ./scripts/build.sh
   ```

### Python dependency issues
If you encounter Python module errors:

1. Activate the virtual environment:
   ```bash
   cd ~/zephyrproject
   source .venv/bin/activate
   pip install -r zephyr/scripts/requirements.txt
   ```
2. Then run the build script again

### Device not detected
- Check USB connection
- Verify device appears in `lsusb` output (Linux) or System Information (macOS)
- Check permissions: `sudo usermod -a -G dialout $USER` (Linux, then log out and back in)

### Build errors
- Clean build: Remove the build directory in Zephyr workspace
  ```bash
  rm -rf ~/zephyrproject/build && ./scripts/build.sh
  ```
- Verify all environment variables are set correctly
- Ensure Zephyr SDK is properly installed

### West command not found
- Ensure west is installed: `pip3 install --user -U west`
- Add Python user bin to PATH: `export PATH=$PATH:~/.local/bin`

### No output on serial
- Check baud rate is 115200
- Verify correct serial port
  - Linux: `/dev/ttyACM0` or `/dev/ttyUSB0`
  - macOS: `/dev/tty.usbmodem*`
- Try resetting the device
- Check that the correct USB port is connected (use the one labeled "J-Link")

### DW3000 initialization fails
- Check SPI connections
- Verify device tree overlay is correct
- Check power supply (should be USB powered)
- Verify GPIO initialization in the overlay file

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
