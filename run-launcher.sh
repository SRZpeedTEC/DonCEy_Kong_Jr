#!/bin/bash
# run-launcher.sh - Run the game launcher

# Check if client is compiled
if [ ! -f "build/launcher" ]; then
    echo "Error: Launcher not compiled. Run ./build.sh first."
    exit 1
fi

echo "================================================"
echo "Starting DonCEy Kong Jr Launcher"
echo "================================================"
echo ""

# Run the launcher
./build/launcher
