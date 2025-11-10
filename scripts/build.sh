#!/bin/bash
# Build script for UWB Scanner

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "======================================"
echo "UWB Scanner Build Script"
echo "======================================"
echo ""

# Check if Zephyr environment is set up
if [ -z "$ZEPHYR_BASE" ]; then
    echo "Error: ZEPHYR_BASE not set"
    echo "Please set up Zephyr environment first:"
    echo "  source <zephyr-workspace>/zephyr/zephyr-env.sh"
    exit 1
fi

echo "Zephyr base: $ZEPHYR_BASE"
echo "Project dir: $PROJECT_DIR"
echo ""

# Build the project
echo "Building..."
west build -b dwm3001cdk "$PROJECT_DIR" --pristine

echo ""
echo "======================================"
echo "Build complete!"
echo "======================================"
echo ""
echo "To flash:"
echo "  west flash"
echo ""
echo "To open serial monitor:"
echo "  python3 scripts/monitor.py /dev/ttyACM0"
