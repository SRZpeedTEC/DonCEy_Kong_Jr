#!/bin/bash
# run-server.sh - Run the Java game server

# Check if server is compiled
if [ ! -d "out" ]; then
    echo "Error: Server not compiled. Run ./build.sh first."
    exit 1
fi

echo "================================================"
echo "Starting DonCEy Kong Jr Game Server"
echo "================================================"
echo "Server will listen on port 9090"
echo "Press Ctrl+C to stop"
echo ""

# Run the server
java -cp out serverJava.GameServer
