@echo off
title Building DeadUnlock
color 0A

echo ========================================
echo   Building DeadUnlock C++ Cheat
echo ========================================
echo.

set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd "%BUILD_DIR%"

echo Generating Visual Studio solution...
cmake "%PROJECT_DIR%" -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo Failed to generate solution!
    pause
    exit /b 1
)

echo.
echo Building Release...
cmake --build . --config Release --target DeadUnlock

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   BUILD SUCCESSFUL!
echo ========================================
echo.
echo Executable: %BUILD_DIR%\Release\DeadUnlock.exe
echo.
pause