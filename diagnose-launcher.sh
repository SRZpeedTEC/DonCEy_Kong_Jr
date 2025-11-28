#!/bin/bash
# diagnose-launcher.sh - Check where launcher is running from and what it can see

echo "=== Launcher Diagnostic ==="
echo ""

echo "1. Checking .app bundle script:"
if [ -f "DonCEy Launcher.app/Contents/MacOS/DonCEy_Launcher" ]; then
    echo "   Script contents:"
    cat "DonCEy Launcher.app/Contents/MacOS/DonCEy_Launcher"
    echo ""
else
    echo "   ❌ DonCEy Launcher.app not found!"
fi

echo "2. Checking if launcher executable exists:"
if [ -f "build/launcher" ]; then
    echo "   ✅ build/launcher exists"
else
    echo "   ❌ build/launcher NOT found!"
fi

echo ""
echo "3. Checking for asset directories:"
if [ -d "clientC/UI/Sprites" ]; then
    echo "   ✅ clientC/UI/Sprites exists"
    echo "      Contents:" $(ls clientC/UI/Sprites/ 2>/dev/null | head -5)
else
    echo "   ❌ clientC/UI/Sprites NOT found!"
fi

if [ -d "clientC/UI/Render" ]; then
    echo "   ✅ clientC/UI/Render exists"
else
    echo "   ❌ clientC/UI/Render NOT found!"
fi

if [ -d "clientC/UI/Game" ]; then
    echo "   ✅ clientC/UI/Game exists"
else
    echo "   ❌ clientC/UI/Game NOT found!"
fi

echo ""
echo "4. Testing launcher from command line:"
echo "   Current directory: $(pwd)"
echo "   Running: ./build/launcher"
echo ""
echo "   If this works but .app doesn't, it's a working directory issue."
echo "   Press Ctrl+C to stop the launcher..."
echo ""

# Uncomment to test:
# ./build/launcher
