#!/usr/bin/env python3
import sys
import os
import argparse
import subprocess

# Default partition settings
PARTITION_NAME = "wad"
PARTITION_OFFSET = "0x3F0000"  # Must match partitions.csv
PARTITION_SIZE = "0x100000"    # 1MB

def main():
    parser = argparse.ArgumentParser(description="Flash DOOM WAD file to ESP32")
    parser.add_argument("wad_file", help="Path to doom1.wad file")
    parser.add_argument("--port", "-p", help="Serial port (optional)")
    parser.add_argument("--baud", "-b", default="460800", help="Baud rate")
    args = parser.parse_args()

    wad_path = args.wad_file
    if not os.path.exists(wad_path):
        print(f"Error: File '{wad_path}' not found.")
        sys.exit(1)

    print(f"Flashing '{wad_path}' to partition '{PARTITION_NAME}' at {PARTITION_OFFSET}...")
    
    # Construct esptool command
    # esptool.py -p PORT -b BAUD write_flash OFFSET FILE
    cmd = ["esptool.py"]
    if args.port:
        cmd.extend(["-p", args.port])
    
    cmd.extend(["-b", args.baud, "write_flash", PARTITION_OFFSET, wad_path])
    
    print("Running command:", " ".join(cmd))
    
    try:
        subprocess.check_call(cmd)
        print("\nSuccess! WAD file flashed.")
        print("Note: If the partition table offset changed, update PARTITION_OFFSET in this script.")
    except subprocess.CalledProcessError:
        print("\nError: Flashing failed.")
        sys.exit(1)
    except FileNotFoundError:
        print("\nError: esptool.py not found. Is it in your PATH?")
        sys.exit(1)

if __name__ == "__main__":
    main()
