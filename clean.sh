#!/bin/bash
# clean.sh - Clean all build artifacts

echo "Cleaning build artifacts..."

# Remove Java build directory
if [ -d "out" ]; then
    echo "  → Removing Java output directory (out/)"
    rm -rf out
fi

# Remove C build directory
if [ -d "build" ]; then
    echo "  → Removing C build directory (build/)"
    rm -rf build
fi

# Remove CMake cache
if [ -f "CMakeCache.txt" ]; then
    rm CMakeCache.txt
fi

# Remove CMake files
if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
fi

echo "✓ Clean complete!"
