#!/bin/bash
# create-macos-apps.sh - macOS .app bundles with Python wrapper and proper cwd

set -e

echo "Creating macOS Application Bundles..."

# --------------------------------------------
# Helper: resolve project root (directory of this script)
# --------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"

echo "Project directory detected as: ${PROJECT_DIR}"
echo ""

# ============================================
# 1. Create DonCEy Server.app
# ============================================
echo "Creating DonCEy Server.app..."

rm -rf "DonCEy Server.app"
mkdir -p "DonCEy Server.app/Contents/MacOS"
mkdir -p "DonCEy Server.app/Contents/Resources"

cat > "DonCEy Server.app/Contents/MacOS/DonCEy_Server" << 'EOF'
#!/usr/bin/env python3
import os, sys, datetime, subprocess

LOG_PATH = "/tmp/doncey-server.log"

def log(msg):
    with open(LOG_PATH, "a") as f:
        f.write(msg + "\n")
    print(msg)

def main():
    # Fresh log
    with open(LOG_PATH, "w") as f:
        f.write("=== DonCEy Server Debug ===\n")
        f.write("Date: " + str(datetime.datetime.now()) + "\n")

    # Locate project directory (3 levels up from this script)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))

    log(f"Script location: {script_dir}")
    log(f"Project directory: {project_dir}")

    # Change working directory so relative paths (out, serverJava, etc.) work
    try:
        os.chdir(project_dir)
    except Exception as e:
        log(f"❌ Failed to chdir to project dir: {e}")
        sys.exit(1)

    log(f"Current working dir: {os.getcwd()}")

    # Command to run the Java server - adjust if your main class/package differs
    cmd = ["java", "-cp", "out", "serverJava.GameServer"]

    log("About to launch server with command: " + " ".join(cmd))

    try:
        # Replace this process with the server process
        os.execvp(cmd[0], cmd)
    except Exception as e:
        log(f"❌ Failed to exec server: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
EOF

chmod +x "DonCEy Server.app/Contents/MacOS/DonCEy_Server"

# ============================================
# 2. Create DonCEy Launcher.app
# ============================================
echo "Creating DonCEy Launcher.app..."

rm -rf "DonCEy Launcher.app"
mkdir -p "DonCEy Launcher.app/Contents/MacOS"
mkdir -p "DonCEy Launcher.app/Contents/Resources"

cat > "DonCEy Launcher.app/Contents/MacOS/DonCEy_Launcher" << 'EOF'
#!/usr/bin/env python3
import os, sys, datetime

LOG_PATH = "/tmp/doncey-launcher.log"

def log(msg):
    with open(LOG_PATH, "a") as f:
        f.write(msg + "\n")
    print(msg)

def main():
    # Fresh log
    with open(LOG_PATH, "w") as f:
        f.write("=== DonCEy Launcher Debug ===\n")
        f.write("Date: " + str(datetime.datetime.now()) + "\n")

    # Locate project directory (3 levels up from this script)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))

    log(f"Script location: {script_dir}")
    log(f"Project directory: {project_dir}")

    # Path to the compiled launcher executable
    launcher_path = os.path.join(project_dir, "build", "launcher")
    log(f"Launcher path: {launcher_path}")

    if not os.path.isfile(launcher_path):
        log("❌ Launcher executable not found. Did you run ./build.sh?")
        sys.exit(1)

    # Change working directory so relative assets (clientC/UI/...) work
    try:
        os.chdir(project_dir)
    except Exception as e:
        log(f"❌ Failed to chdir to project dir: {e}")
        sys.exit(1)

    log(f"Current working dir: {os.getcwd()}")
    log("Executing launcher...")

    try:
        # Replace this wrapper process with the real launcher
        os.execv(launcher_path, [launcher_path])
    except Exception as e:
        log(f"❌ Failed to exec launcher: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
EOF

chmod +x "DonCEy Launcher.app/Contents/MacOS/DonCEy_Launcher"

echo ""
echo "✓ Created DonCEy Server.app"
echo "✓ Created DonCEy Launcher.app (Python wrapper with cwd fix)"
echo ""
echo "IMPORTANT (macOS security):"
echo "  xattr -cr \"DonCEy Server.app\""
echo "  xattr -cr \"DonCEy Launcher.app\""
echo ""
echo "You can now double-click the apps!"
