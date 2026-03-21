@echo off
REM Tonka Construction Launcher for Modern Windows
REM Deploys the WING32.dll shim and launches the original 32-bit TONKA.EXE

setlocal

set "PATCH_DIR=%~dp0"
set "GAME_DIR=%~dp0..\.tonka\DATA"

REM Check for required files
if not exist "%GAME_DIR%\TONKA.EXE" (
    echo ERROR: TONKA.EXE not found in %GAME_DIR%
    echo Place original Tonka Construction disc contents in .tonka\
    pause
    exit /b 1
)
if not exist "%GAME_DIR%\CW3215.DLL" (
    echo ERROR: CW3215.DLL not found in %GAME_DIR%
    pause
    exit /b 1
)

REM Deploy our WING32.dll shim (handles INI path fix automatically)
if not exist "%PATCH_DIR%WING32.dll" (
    echo ERROR: WING32.dll shim not found. Run build.sh first.
    pause
    exit /b 1
)
copy /Y "%PATCH_DIR%WING32.dll" "%GAME_DIR%\WING32.dll" >nul

REM Launch the game from its directory
cd /d "%GAME_DIR%"
echo Starting Tonka Construction...
start "" TONKA.EXE

endlocal
