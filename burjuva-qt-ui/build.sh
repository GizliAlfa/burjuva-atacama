#!/bin/bash

# Burjuva-Atacama Qt UI Build Script
# For Linux/Raspberry Pi

set -e

echo "=========================================="
echo "Burjuva-Atacama Qt UI Build Script"
echo "=========================================="

# Check for Qt5
if ! pkg-config --exists Qt5Core; then
    echo "ERROR: Qt5 not found!"
    echo "Install with: sudo apt install qtbase5-dev libqt5serialport5-dev"
    exit 1
fi

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Run CMake
echo "Running CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="
echo "Executable: $(pwd)/burjuva-ui"
echo ""
echo "To run:"
echo "  cd build"
echo "  ./burjuva-ui"
echo ""
