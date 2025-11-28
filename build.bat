@echo off
REM build.bat - Build both client (C) and server (Java) on Windows

echo ================================================
echo Building DonCEy Kong Jr - Complete Project
echo ================================================

REM ============================================
REM 1. Build Java Server
REM ============================================
echo.
echo [1/2] Building Java Server...

REM Clean and create output directory
if exist out rmdir /s /q out
mkdir out

REM Compile all Java files
echo   - Compiling Java sources...
for /r serverJava %%f in (*.java) do (
    if not defined JAVA_FILES (
        set "JAVA_FILES=%%f"
    ) else (
        set "JAVA_FILES=!JAVA_FILES! %%f"
    )
)

javac --release 17 -d out %JAVA_FILES%
if %ERRORLEVEL% NEQ 0 (
    echo   X Java compilation failed
    exit /b 1
)
echo   √ Java server compiled successfully

REM ============================================
REM 2. Build C Client
REM ============================================
echo.
echo [2/2] Building C Client...

REM Create build directory if it doesn't exist
if not exist build (
    echo   - Creating build directory...
    mkdir build
    cd build
    echo   - Running CMake configuration...
    cmake ..
    cd ..
)

REM Build the client
echo   - Compiling C sources...
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo   X C compilation failed
    exit /b 1
)
echo   √ C client compiled successfully

REM ============================================
REM Summary
REM ============================================
echo.
echo ================================================
echo Build Complete!
echo ================================================
echo.
echo Executables created:
echo   Java Server:  java -cp out serverJava.GameServer
echo   C Launcher:   build\Release\launcher.exe (or build\launcher.exe)
echo   C Player:     build\Release\player.exe
echo   C Spectator:  build\Release\spectator.exe
echo.
echo Quick Start:
echo   1. Terminal 1: run-server.bat
echo   2. Terminal 2: run-launcher.bat
echo.

REM ============================================
REM Create Windows Shortcuts
REM ============================================
echo Creating clickable shortcuts...
if exist create-windows-shortcuts.bat (
    call create-windows-shortcuts.bat
    echo.
    echo √ Created Windows shortcuts!
    echo   - Double-click "DonCEy Server.vbs" to start server
    echo   - Double-click "DonCEy Launcher.vbs" to start launcher
    echo.
)

pause