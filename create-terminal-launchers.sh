#!/bin/bash
# create-terminal-launchers.sh
# Creates AppleScript .app bundles that open Terminal and run make commands

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Creating Terminal Launcher Apps..."
echo "Project directory: $PROJECT_DIR"
echo ""

# ============================================
# 1. Start Server.app
# ============================================
echo "Creating Start Server.app..."

rm -rf "Start Server.app"
mkdir -p "Start Server.app/Contents/MacOS"
mkdir -p "Start Server.app/Contents/Resources"

cat > "Start Server.app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Start_Server</string>
    <key>CFBundleName</key>
    <string>Start Server</string>
    <key>CFBundleIdentifier</key>
    <string>com.doncey.startserver</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
EOF

cat > "Start Server.app/Contents/MacOS/Start_Server" <<EOF
#!/bin/bash
# Open Terminal and run server

osascript <<APPLESCRIPT
tell application "Terminal"
    activate
    do script "cd '$PROJECT_DIR' && make run-server"
end tell
APPLESCRIPT
EOF

chmod +x "Start Server.app/Contents/MacOS/Start_Server"

# ============================================
# 2. Start Launcher.app
# ============================================
echo "Creating Start Launcher.app..."

rm -rf "Start Launcher.app"
mkdir -p "Start Launcher.app/Contents/MacOS"
mkdir -p "Start Launcher.app/Contents/Resources"

cat > "Start Launcher.app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Start_Launcher</string>
    <key>CFBundleName</key>
    <string>Start Launcher</string>
    <key>CFBundleIdentifier</key>
    <string>com.doncey.startlauncher</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
EOF

cat > "Start Launcher.app/Contents/MacOS/Start_Launcher" <<EOF
#!/bin/bash
# Open Terminal and run launcher

osascript <<APPLESCRIPT
tell application "Terminal"
    activate
    do script "cd '$PROJECT_DIR' && make run-launcher"
end tell
APPLESCRIPT
EOF

chmod +x "Start Launcher.app/Contents/MacOS/Start_Launcher"

# ============================================
# 3. Start Game.app (runs both server + launcher)
# ============================================
echo "Creating Start Game.app..."

rm -rf "Start Game.app"
mkdir -p "Start Game.app/Contents/MacOS"
mkdir -p "Start Game.app/Contents/Resources"

cat > "Start Game.app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Start_Game</string>
    <key>CFBundleName</key>
    <string>Start Game</string>
    <key>CFBundleIdentifier</key>
    <string>com.doncey.startgame</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
EOF

cat > "Start Game.app/Contents/MacOS/Start_Game" <<EOF
#!/bin/bash
# Open two Terminal tabs: one for server, one for launcher

osascript <<APPLESCRIPT
tell application "Terminal"
    activate
    
    -- Start server in first tab
    do script "cd '$PROJECT_DIR' && echo 'Starting DonCEy Kong Jr Server...' && make run-server"
    
    -- Wait a moment for server to start
    delay 2
    
    -- Start launcher in new tab
    tell application "System Events" to keystroke "t" using command down
    delay 0.5
    do script "cd '$PROJECT_DIR' && echo 'Starting DonCEy Kong Jr Launcher...' && make run-launcher" in window 1
end tell
APPLESCRIPT
EOF

chmod +x "Start Game.app/Contents/MacOS/Start_Game"

# ============================================
# Remove quarantine
# ============================================
echo ""
echo "Removing quarantine attributes..."
xattr -cr "Start Server.app" 2>/dev/null || true
xattr -cr "Start Launcher.app" 2>/dev/null || true
xattr -cr "Start Game.app" 2>/dev/null || true

echo ""
echo "================================================"
echo "✓ Created Start Server.app"
echo "✓ Created Start Launcher.app"
echo "✓ Created Start Game.app (runs both!)"
echo "================================================"
echo ""
echo "How to use:"
echo "  1. Double-click 'Start Game.app' to launch everything"
echo "     OR"
echo "  1. Double-click 'Start Server.app'"
echo "  2. Double-click 'Start Launcher.app'"
echo ""
echo "These apps open Terminal and run the make commands."
echo "Much simpler and more reliable!"
echo ""
