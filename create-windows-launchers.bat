@echo off
REM create-windows-launchers.bat
REM Creates Windows batch file launchers for DonCEy Kong Jr

echo Creating Windows Launcher Scripts...
echo.

REM Get the current directory (project root)
set "PROJECT_DIR=%CD%"
echo Project directory: %PROJECT_DIR%
echo.

REM ============================================
REM 1. Start Server.bat
REM ============================================
echo Creating Start Server.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Server Launcher
echo title DonCEy Kong Jr - Server
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo    DonCEy Kong Jr - Starting Server
echo echo ==========================================
echo echo.
echo mingw32-make run-server
echo echo.
echo echo Server stopped.
echo pause
) > "Start Server.bat"

REM ============================================
REM 2. Start Launcher.bat
REM ============================================
echo Creating Start Launcher.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Launcher
echo title DonCEy Kong Jr - Launcher
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo    DonCEy Kong Jr - Starting Launcher
echo echo ==========================================
echo echo.
echo mingw32-make run-launcher
echo echo.
echo echo Launcher closed.
echo pause
) > "Start Launcher.bat"

REM ============================================
REM 3. Start Game.bat (runs both server + launcher)
REM ============================================
echo Creating Start Game.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Complete Game Launcher
echo title DonCEy Kong Jr
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo      DonCEy Kong Jr - Starting Game
echo echo ==========================================
echo echo.
echo echo Starting server in new window...
echo start "DonCEy Server" cmd /k "cd /d "%PROJECT_DIR%" && mingw32-make run-server"
echo echo.
echo echo Waiting 3 seconds for server to start...
echo timeout /t 3 /nobreak ^>nul
echo echo.
echo echo Starting launcher...
echo mingw32-make run-launcher
echo echo.
echo echo Launcher closed.
echo pause
) > "Start Game.bat"

REM ============================================
REM 4. Build.bat (convenience build script)
REM ============================================
echo Creating Build.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Build Script
echo title DonCEy Kong Jr - Building
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo    DonCEy Kong Jr - Building Project
echo echo ==========================================
echo echo.
echo mingw32-make clean
echo echo.
echo mingw32-make
echo echo.
echo echo Build complete!
echo echo.
echo pause
) > "Build.bat"

REM ============================================
REM 5. Start Player.bat (direct player client)
REM ============================================
echo Creating Start Player.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Player Client
echo title DonCEy Kong Jr - Player
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo    DonCEy Kong Jr - Player Client
echo echo ==========================================
echo echo.
echo set /p SERVER_IP="Enter server IP (default 127.0.0.1): "
echo if "%%SERVER_IP%%"=="" set SERVER_IP=127.0.0.1
echo echo.
echo set /p SERVER_PORT="Enter server port (default 9090): "
echo if "%%SERVER_PORT%%"=="" set SERVER_PORT=9090
echo echo.
echo echo Connecting to %%SERVER_IP%%:%%SERVER_PORT%%...
echo mingw32-make run-player IP=%%SERVER_IP%% PORT=%%SERVER_PORT%%
echo echo.
echo echo Player disconnected.
echo pause
) > "Start Player.bat"

REM ============================================
REM 6. Start Spectator.bat (direct spectator client)
REM ============================================
echo Creating Start Spectator.bat...

(
echo @echo off
echo REM DonCEy Kong Jr - Spectator Client
echo title DonCEy Kong Jr - Spectator
echo cd /d "%PROJECT_DIR%"
echo echo.
echo echo ==========================================
echo echo    DonCEy Kong Jr - Spectator Client
echo echo ==========================================
echo echo.
echo set /p SERVER_IP="Enter server IP (default 127.0.0.1): "
echo if "%%SERVER_IP%%"=="" set SERVER_IP=127.0.0.1
echo echo.
echo set /p SERVER_PORT="Enter server port (default 9090): "
echo if "%%SERVER_PORT%%"=="" set SERVER_PORT=9090
echo echo.
echo set /p SLOT="Enter player slot to spectate (1 or 2, default 1): "
echo if "%%SLOT%%"=="" set SLOT=1
echo echo.
echo echo Connecting to %%SERVER_IP%%:%%SERVER_PORT%% (slot %%SLOT%%)...
echo mingw32-make run-spectator IP=%%SERVER_IP%% PORT=%%SERVER_PORT%% SLOT=%%SLOT%%
echo echo.
echo echo Spectator disconnected.
echo pause
) > "Start Spectator.bat"

echo.
echo ================================================
echo Created the following launcher files:
echo ================================================
echo.
echo   Start Game.bat       - Launch everything (server + launcher)
echo   Start Server.bat     - Server only
echo   Start Launcher.bat   - Launcher only
echo   Start Player.bat     - Player client only
echo   Start Spectator.bat  - Spectator client only
echo   Build.bat            - Build the project
echo.
echo ================================================
echo How to use:
echo ================================================
echo.
echo   Quick Start: Double-click "Start Game.bat"
echo.
echo   Manual:
echo     1. Double-click "Start Server.bat"
echo     2. Double-click "Start Launcher.bat"
echo.
echo   Build first if needed: Double-click "Build.bat"
echo.
echo ================================================
echo.
pause
