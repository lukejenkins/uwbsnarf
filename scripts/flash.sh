#!/bin/bash
# Flash script for UWB Scanner

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "======================================"
echo "UWB Scanner Flash Script"
echo "======================================"
echo ""

# Auto-detect Zephyr workspace
if [ -z "$ZEPHYR_BASE" ]; then
    # Try common locations
    if [ -d "$HOME/zephyrproject/zephyr" ]; then
        ZEPHYR_WORKSPACE="$HOME/zephyrproject"
    elif [ -d "$HOME/zephyr-workspace/zephyr" ]; then
        ZEPHYR_WORKSPACE="$HOME/zephyr-workspace"
    else
        echo "Error: Could not find Zephyr workspace"
        echo "Please set ZEPHYR_BASE or ensure Zephyr is installed at:"
        echo "  ~/zephyrproject/ or ~/zephyr-workspace/"
        echo ""
        echo "Or source the Zephyr environment manually:"
        echo "  source <zephyr-workspace>/zephyr/zephyr-env.sh"
        exit 1
    fi

    echo "Found Zephyr workspace: $ZEPHYR_WORKSPACE"

    # Activate Python virtual environment if it exists
    if [ -d "$ZEPHYR_WORKSPACE/.venv" ]; then
        echo "Activating Python virtual environment..."
        source "$ZEPHYR_WORKSPACE/.venv/bin/activate"
    fi

    # Source Zephyr environment
    echo "Setting up Zephyr environment..."
    source "$ZEPHYR_WORKSPACE/zephyr/zephyr-env.sh"

    # Auto-detect Zephyr SDK
    if [ -z "$ZEPHYR_SDK_INSTALL_DIR" ]; then
        if [ -d "$ZEPHYR_WORKSPACE/zephyr-sdk-0.17.0" ]; then
            export ZEPHYR_SDK_INSTALL_DIR="$ZEPHYR_WORKSPACE/zephyr-sdk-0.17.0"
            echo "Found Zephyr SDK: $ZEPHYR_SDK_INSTALL_DIR"
        elif [ -d "$HOME/zephyr-sdk-0.17.0" ]; then
            export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr-sdk-0.17.0"
            echo "Found Zephyr SDK: $ZEPHYR_SDK_INSTALL_DIR"
        fi
    fi
fi

# Check if build exists
BUILD_DIR="$(dirname $ZEPHYR_BASE)/build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found at $BUILD_DIR"
    echo "Please build the project first:"
    echo "  ./scripts/build.sh"
    exit 1
fi

echo "Build dir: $BUILD_DIR"
echo ""

# Flash the device from Zephyr workspace
echo "Flashing device..."
cd "$ZEPHYR_BASE/.."
west flash

echo ""
echo "======================================"
echo "Flash complete!"
echo "======================================"
echo ""
echo "To monitor output:"
echo "  python3 $PROJECT_DIR/scripts/monitor.py /dev/ttyACM0"
echo "  (or /dev/tty.usbmodem* on macOS)"
