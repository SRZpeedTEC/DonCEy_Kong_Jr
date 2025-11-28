@echo off
REM run-server.bat - Run the Java game server on Windows

if not exist out (
    echo Error: Server not compiled. Run build.bat first.
    pause
    exit /b 1
)

echo ================================================
echo Starting DonCEy Kong Jr Game Server
echo ================================================
echo Server will listen on port 9090
echo Press Ctrl+C to stop
echo.

java -cp out serverJava.GameServer
pause
