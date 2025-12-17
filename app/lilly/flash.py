#!/usr/bin/env python3
"""
Quick flash script for Lilly ornament.
Sends reboot command over USB serial, then copies UF2 to the Pico.

Usage:
    python flash.py [COM_PORT] [UF2_PATH]

Examples:
    python flash.py                     # Auto-detect COM port, use default UF2 path
    python flash.py COM3                # Specify COM port
    python flash.py COM3 path/to/zephyr.uf2
"""

import sys
import time
import shutil
import serial
import serial.tools.list_ports
from pathlib import Path

# Default paths
DEFAULT_UF2 = Path(__file__).parent.parent.parent.parent / "build" / "zephyr" / "zephyr.uf2"
PICO_DRIVE_NAMES = ["RPI-RP2", "RP2350"]


def find_pico_com_port():
    """Find the Pico's CDC ACM COM port."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for Raspberry Pi vendor ID or description
        if port.vid == 0x2E8A or "Pico" in (port.description or ""):
            return port.device
        # Also check for our custom description
        if "Lilly" in (port.description or "") or "Christmas" in (port.description or ""):
            return port.device
    return None


def find_pico_drive():
    """Find the Pico UF2 drive (Windows)."""
    import string
    for letter in string.ascii_uppercase:
        drive = Path(f"{letter}:")
        if drive.exists():
            # Check for INFO_UF2.TXT which indicates a UF2 bootloader drive
            info_file = drive / "INFO_UF2.TXT"
            if info_file.exists():
                return drive
    return None


def reboot_to_bootloader(com_port):
    """Send reboot command to Pico."""
    print(f"Sending reboot command to {com_port}...")
    try:
        ser = serial.Serial(com_port, 115200, timeout=1)
        ser.write(b'R')
        ser.close()
        print("Reboot command sent")
        return True
    except Exception as e:
        print(f"Failed to send reboot command: {e}")
        return False


def wait_for_drive(timeout=10):
    """Wait for Pico UF2 drive to appear."""
    print("Waiting for Pico drive to appear...")
    start = time.time()
    while time.time() - start < timeout:
        drive = find_pico_drive()
        if drive:
            print(f"Found Pico drive at {drive}")
            return drive
        time.sleep(0.5)
    return None


def flash_uf2(uf2_path, drive):
    """Copy UF2 file to Pico drive."""
    dest = drive / uf2_path.name
    print(f"Copying {uf2_path} to {dest}...")
    shutil.copy(uf2_path, dest)
    print("Flash complete!")


def main():
    # Parse arguments
    com_port = None
    uf2_path = DEFAULT_UF2

    if len(sys.argv) >= 2:
        com_port = sys.argv[1]
    if len(sys.argv) >= 3:
        uf2_path = Path(sys.argv[2])

    # Verify UF2 exists
    if not uf2_path.exists():
        print(f"Error: UF2 file not found: {uf2_path}")
        print("Did you run 'west build' first?")
        sys.exit(1)

    print(f"UF2 file: {uf2_path}")

    # Check if drive is already in bootloader mode
    drive = find_pico_drive()
    if drive:
        print(f"Pico already in bootloader mode at {drive}")
        flash_uf2(uf2_path, drive)
        return

    # Find COM port if not specified
    if not com_port:
        com_port = find_pico_com_port()
        if not com_port:
            print("Error: Could not find Pico COM port")
            print("Available ports:")
            for port in serial.tools.list_ports.comports():
                print(f"  {port.device}: {port.description} (VID={port.vid}, PID={port.pid})")
            sys.exit(1)

    print(f"Using COM port: {com_port}")

    # Reboot to bootloader
    if not reboot_to_bootloader(com_port):
        sys.exit(1)

    # Wait for drive
    time.sleep(1)  # Give it a moment to reboot
    drive = wait_for_drive()
    if not drive:
        print("Error: Pico drive did not appear")
        print("Try manually holding BOOTSEL and pressing reset")
        sys.exit(1)

    # Flash
    flash_uf2(uf2_path, drive)


if __name__ == "__main__":
    main()
