@echo off
REM run-launcher.bat - Run the game launcher on Windows

REM Check both possible locations (Debug/Release and root)
if exist build\Release\launcher.exe (
    set LAUNCHER=build\Release\launcher.exe
) else if exist build\launcher.exe (
    set LAUNCHER=build\launcher.exe
) else if exist build\Debug\launcher.exe (
    set LAUNCHER=build\Debug\launcher.exe
) else (
    echo Error: Launcher not compiled. Run build.bat first.
    pause
    exit /b 1
)

echo ================================================
echo Starting DonCEy Kong Jr Launcher
echo ================================================
echo.

%LAUNCHER%
