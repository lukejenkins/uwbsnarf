#!/usr/bin/env python3
"""
UWB Scanner Monitor
Connects to the UWB scanner via serial port and displays device information
"""

import serial
import json
import sys
import argparse
from datetime import datetime


def parse_device_info(data):
    """Parse JSON device information"""
    try:
        return json.loads(data)
    except json.JSONDecodeError:
        return None


def format_device_info(info):
    """Format device information for display"""
    if info.get('type') == 'device_found':
        addr = info.get('device_addr', 'Unknown')
        dist = info.get('distance_cm', 0)
        rssi = info.get('rssi_dbm', 0)
        timestamp = info.get('timestamp_ms', 0)
        channel = info.get('channel', 0)
        prf = info.get('prf', 0)
        quality = info.get('frame_quality', 0)

        print(f"\n{'='*60}")
        print(f"Device Found: 0x{addr}")
        print(f"{'='*60}")
        print(f"Timestamp:      {timestamp} ms")
        print(f"Distance:       {dist:.2f} cm ({dist/100:.2f} m)")
        print(f"RSSI:           {rssi:.2f} dBm")
        print(f"Channel:        {channel}")
        print(f"PRF:            {prf} MHz")
        print(f"Frame Quality:  {quality}")
        print(f"{'='*60}")

    elif info.get('type') == 'status':
        msg = info.get('message', '')
        print(f"[{datetime.now().strftime('%H:%M:%S')}] STATUS: {msg}")

    elif info.get('type') == 'error':
        msg = info.get('message', '')
        print(f"[{datetime.now().strftime('%H:%M:%S')}] ERROR: {msg}")


def main():
    parser = argparse.ArgumentParser(description='UWB Scanner Monitor')
    parser.add_argument('port', nargs='?', default='/dev/ttyACM0',
                       help='Serial port (default: /dev/ttyACM0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200,
                       help='Baud rate (default: 115200)')
    parser.add_argument('-o', '--output', type=str,
                       help='Output file for logging')

    args = parser.parse_args()

    # Open output file if specified
    output_file = None
    if args.output:
        output_file = open(args.output, 'w')
        print(f"Logging to: {args.output}")

    ser = None

    try:
        # Open serial port
        print(f"Connecting to {args.port} at {args.baudrate} baud...")
        ser = serial.Serial(args.port, args.baudrate, timeout=1)
        print("Connected!")
        print("Waiting for device data...\n")

        # Read and process data
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    # Log to file if enabled
                    if output_file:
                        output_file.write(line + '\n')
                        output_file.flush()

                    # Split out any leading non-JSON text (e.g. Zephyr logs)
                    stripped = line.strip()
                    if not stripped:
                        continue

                    json_start = stripped.find('{')
                    if json_start > 0:
                        prefix = stripped[:json_start].strip()
                        if prefix:
                            print(prefix)
                        stripped = stripped[json_start:]
                    elif json_start == -1:
                        print(stripped)
                        continue

                    json_end = stripped.rfind('}')
                    if json_end > -1 and json_end + 1 < len(stripped):
                        suffix = stripped[json_end + 1:].strip()
                        if suffix:
                            print(suffix)
                        stripped = stripped[:json_end + 1]

                    info = parse_device_info(stripped)
                    if info:
                        format_device_info(info)
                    else:
                        # If we still cannot parse, print the raw JSON-ish line once
                        print(stripped)

            except UnicodeDecodeError:
                # Skip non-UTF8 data
                pass

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\nExiting...")

    finally:
        if output_file:
            output_file.close()
        if ser:
            ser.close()


if __name__ == '__main__':
    main()
