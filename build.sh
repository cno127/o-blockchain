#!/bin/bash

# My Bitcoin Project Build Script
# This script builds the custom Bitcoin-based project

set -e

echo "Building My Bitcoin Project..."

# Create build directory
mkdir -p build

# Configure with CMake
echo "Configuring build..."
cmake -B build

# Build the project
echo "Building project..."
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build completed successfully!"
echo ""
echo "To run the daemon:"
echo "  ./build/bin/bitcoind"
echo ""
echo "To run the CLI:"
echo "  ./build/bin/bitcoin-cli"
echo ""
echo "To run the GUI (if built):"
echo "  ./build/bin/bitcoin-qt" 