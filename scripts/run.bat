@echo off
title DeadUnlock
color 0A

echo ========================================
echo   Running DeadUnlock
echo ========================================
echo.
echo Controls:
echo   F1     - Toggle Aimbot
echo   INS    - Toggle Menu
echo   END    - Exit
echo.
echo Make sure Deadlock is RUNNING!
echo.

cd /d "%~dp0..\build\Release"

if not exist "DeadUnlock.exe" (
    echo Executable not found! Run build.bat first.
    pause
    exit /b 1
)

DeadUnlock.exe

pause