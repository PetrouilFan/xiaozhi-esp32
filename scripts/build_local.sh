#!/bin/bash
# xiaozhi-esp32 local build script for Waveshare ESP32-S3 Touch LCD 4.3" Board

set -e

echo "=== XiaoZhi ESP32 Local Build Script ==="
echo ""

# Check dependencies
command -v python3 >/dev/null 2>&1 || { echo "Error: python3 not found"; exit 1; }

# Set up ESP-IDF if needed
if [ -z "$IDF_PATH" ]; then
    ESPIDF_DIR="$HOME/esp-idf"
    if [ -d "$ESPIDF_DIR" ]; then
        echo "Found ESP-IDF at $ESPIDF_DIR, sourcing..."
        . "$ESPIDF_DIR/export.sh"
    else
        echo "Error: ESP-IDF not found at $ESPIDF_DIR"
        echo "Please install ESP-IDF v5.5.2+: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started.html"
        exit 1
    fi
fi

# Check we're in the right directory
cd "$(dirname "$0")"

echo "Current directory: $(pwd)"
echo "ESP-IDF path: $IDF_PATH"
echo ""

# Clean previous build
echo "=== Cleaning previous build ==="
rm -rf build
echo ""

# Set target to ESP32-S3
echo "=== Setting target to ESP32-S3 ==="
idf.py set-target esp32s3
echo ""

# Build for your specific board
echo "=== Building for waveshare-esp32-s3-touch-lcd-4.3c ==="
python scripts/release.py waveshare/esp32-s3-touch-lcd-4.3c --name esp32-s3-touch-lcd-4.3c
echo ""

# Check result
if [ -f "build/merged-binary.bin" ]; then
    echo "=== Build SUCCESS ==="
    echo "Binary: build/merged-binary.bin"
    ls -lh build/merged-binary.bin
else
    echo "Build failed - check errors above"
    exit 1
fi

echo ""
echo "=== Done! ==="