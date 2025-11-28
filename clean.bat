@echo off
REM clean.bat - Clean all build artifacts on Windows

echo Cleaning build artifacts...

if exist out (
    echo   - Removing Java output directory (out\)
    rmdir /s /q out
)

if exist build (
    echo   - Removing C build directory (build\)
    rmdir /s /q build
)

if exist CMakeCache.txt (
    del CMakeCache.txt
)

if exist CMakeFiles (
    rmdir /s /q CMakeFiles
)

echo √ Clean complete!
pause
