#!/bin/bash
# build.sh - Build both client (C) and server (Java)

set -e  # Exit on error

echo "================================================"
echo "Building DonCEy Kong Jr - Complete Project"
echo "================================================"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# ============================================
# 1. Build Java Server
# ============================================
echo -e "\n${BLUE}[1/2] Building Java Server...${NC}"

# Clean and create output directory
rm -rf out
mkdir -p out

# Compile all Java files
echo "  → Compiling Java sources..."
javac --release 17 -d out $(find serverJava -name "*.java")

if [ $? -eq 0 ]; then
    echo -e "${GREEN}  ✓ Java server compiled successfully${NC}"
else
    echo -e "${RED}  ✗ Java compilation failed${NC}"
    exit 1
fi

# ============================================
# 2. Build C Client
# ============================================
echo -e "\n${BLUE}[2/2] Building C Client...${NC}"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "  → Creating build directory..."
    mkdir build
    cd build
    echo "  → Running CMake configuration..."
    cmake ..
    cd ..
fi

# Build the client
echo "  → Compiling C sources..."
cmake --build build

if [ $? -eq 0 ]; then
    echo -e "${GREEN}  ✓ C client compiled successfully${NC}"
else
    echo -e "${RED}  ✗ C compilation failed${NC}"
    exit 1
fi

# ============================================
# Summary
# ============================================
echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}Build Complete!${NC}"
echo -e "${GREEN}================================================${NC}"

echo -e "\nExecutables created:"
echo "  Java Server:  java -cp out serverJava.GameServer"
echo "  C Launcher:   ./build/launcher"
echo "  C Player:     ./build/player"
echo "  C Spectator:  ./build/spectator"

echo -e "\n${BLUE}Quick Start:${NC}"
echo "  1. Terminal 1: ./run-server.sh"
echo "  2. Terminal 2: ./run-launcher.sh"

# ============================================
# Create Platform-Specific Clickable Launchers
# ============================================
echo -e "\n${BLUE}Creating clickable launchers...${NC}"

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS - Create .app bundles
    if [ -f "create-macos-apps.sh" ]; then
        ./create-macos-apps.sh
        echo -e "${GREEN}✓ Created macOS .app bundles${NC}"
        echo "  → Double-click 'DonCEy Server.app' to start server"
        echo "  → Double-click 'DonCEy Launcher.app' to start launcher"
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux - Create desktop entries
    if [ -f "create-linux-desktop-entries.sh" ]; then
        ./create-linux-desktop-entries.sh
        echo -e "${GREEN}✓ Created Linux desktop entries${NC}"
        echo "  → Find 'DonCEy Kong Jr' in your application menu"
    fi
fi

echo ""