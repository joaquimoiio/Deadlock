@echo off
title DeadUnlock Offset Dumper
color 0A

echo ========================================
echo   DeadUnlock Offset Dumper
echo ========================================
echo.
echo Make sure Deadlock is RUNNING!
echo.
pause

cls
echo Scanning for offsets...
echo.

cd /d "%~dp0"
python offset_finder.py --output ..\..\config\offsets.json --debug

echo.
echo ========================================
if %errorlevel%==0 (
    echo   OFFSETS UPDATED SUCCESSFULLY!
    color 0A
) else (
    echo   ERROR: Failed to update offsets
    color 0C
)
echo ========================================
echo.
echo Press any key to exit...
pause > nul