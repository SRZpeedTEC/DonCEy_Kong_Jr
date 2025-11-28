REM create-windows-shortcuts.bat - Create clickable shortcuts for Windows

@echo off
echo Creating Windows shortcuts...

REM ============================================
REM 1. Create Server shortcut
REM ============================================
echo Creating "DonCEy Server.vbs"...

(
echo Set WshShell = CreateObject^("WScript.Shell"^)
echo.
echo ' Get current directory
echo currentDir = WshShell.CurrentDirectory
echo.
echo ' Check if server is compiled
echo Set fso = CreateObject^("Scripting.FileSystemObject"^)
echo If Not fso.FolderExists^(currentDir ^& "\out"^) Then
echo     MsgBox "Server not compiled!" ^& vbCrLf ^& vbCrLf ^& "Please run build.bat first.", vbCritical, "DonCEy Kong Jr"
echo     WScript.Quit 1
echo End If
echo.
echo ' Run server in new command prompt window
echo WshShell.Run "cmd /k cd /d """ ^& currentDir ^& """ ^&^& echo ================================================ ^&^& echo DonCEy Kong Jr - Game Server ^&^& echo ================================================ ^&^& echo Starting server on port 9090... ^&^& echo Press Ctrl+C to stop ^&^& echo. ^&^& java -cp out serverJava.GameServer", 1, False
) > "DonCEy Server.vbs"

REM ============================================
REM 2. Create Launcher shortcut
REM ============================================
echo Creating "DonCEy Launcher.vbs"...

(
echo Set WshShell = CreateObject^("WScript.Shell"^)
echo.
echo ' Get current directory
echo currentDir = WshShell.CurrentDirectory
echo.
echo ' Check if launcher is compiled
echo Set fso = CreateObject^("Scripting.FileSystemObject"^)
echo launcherPath = ""
echo If fso.FileExists^(currentDir ^& "\build\Release\launcher.exe"^) Then
echo     launcherPath = currentDir ^& "\build\Release\launcher.exe"
echo ElseIf fso.FileExists^(currentDir ^& "\build\launcher.exe"^) Then
echo     launcherPath = currentDir ^& "\build\launcher.exe"
echo ElseIf fso.FileExists^(currentDir ^& "\build\Debug\launcher.exe"^) Then
echo     launcherPath = currentDir ^& "\build\Debug\launcher.exe"
echo End If
echo.
echo If launcherPath = "" Then
echo     MsgBox "Launcher not compiled!" ^& vbCrLf ^& vbCrLf ^& "Please run build.bat first.", vbCritical, "DonCEy Kong Jr"
echo     WScript.Quit 1
echo End If
echo.
echo ' Run launcher ^(no console window^)
echo WshShell.Run """" ^& launcherPath ^& """", 1, False
) > "DonCEy Launcher.vbs"

echo.
echo ================================================
echo Shortcuts Created!
echo ================================================
echo.
echo You can now double-click these files to run:
echo   • DonCEy Server.vbs   - Opens Command Prompt and starts server
echo   • DonCEy Launcher.vbs - Opens game launcher
echo.
echo Optional: Right-click each .vbs file and create a desktop shortcut
echo.
pause
