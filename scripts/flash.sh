#!/bin/bash
# Flash script for UWB Scanner

set -e

echo "======================================"
echo "UWB Scanner Flash Script"
echo "======================================"
echo ""

# Check if build exists
if [ ! -d "build" ]; then
    echo "Error: Build directory not found"
    echo "Please build the project first:"
    echo "  ./scripts/build.sh"
    exit 1
fi

# Flash the device
echo "Flashing device..."
west flash

echo ""
echo "======================================"
echo "Flash complete!"
echo "======================================"
echo ""
echo "To monitor output:"
echo "  python3 scripts/monitor.py /dev/ttyACM0"
