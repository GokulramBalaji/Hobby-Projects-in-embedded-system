@echo off
setlocal enabledelayedexpansion
title AMT Control System - Installation Script
color 0B

echo ==============================================================================
echo       ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM
echo                    Windows Environment Setup & Installer
echo ==============================================================================
echo.

:: 1. Verify Node.js and NPM
echo [1/6] Verifying Node.js and NPM runtime...
where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    color 0C
    echo [ERROR] Node.js is not installed or not in PATH!
    echo Please install Node.js (v18 or newer) from https://nodejs.org/
    pause
    exit /b 1
)
for /f "tokens=*" %%i in ('node -v') do set NODE_VER=%%i
for /f "tokens=*" %%i in ('npm -v') do set NPM_VER=%%i
echo   - Node.js Version: %NODE_VER%
echo   - NPM Version:     %NPM_VER%
echo.

:: 2. Check Mosquitto MQTT Broker
echo [2/6] Checking Mosquitto MQTT Broker...
where mosquitto >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   - Native Mosquitto broker detected in PATH.
) else if exist "C:\Program Files\mosquitto\mosquitto.exe" (
    echo   - Native Mosquitto found in C:\Program Files\mosquitto\
) else (
    echo   - Native Mosquitto not detected in system PATH.
    echo   * Note: The backend includes an embedded Aedes MQTT broker that starts
    echo     automatically on port 1883 if Mosquitto is not installed!
    echo     To install native Mosquitto: download from https://mosquitto.org/download/
)
echo.

:: 3. Create Required Directory Hierarchy
echo [3/6] Initializing storage directories...
if not exist "..\backend\src\database" mkdir "..\backend\src\database"
if not exist "..\mqtt\data" mkdir "..\mqtt\data"
if not exist "..\logs" mkdir "..\logs"
echo   - Created / verified directory structure.
echo.

:: 4. Install Backend Dependencies
echo [4/6] Installing Node.js Backend dependencies...
cd ..\backend
call npm install
if %ERRORLEVEL% NEQ 0 (
    color 0C
    echo [ERROR] Failed to install backend dependencies!
    pause
    exit /b 1
)
cd ..\scripts
echo   - Backend dependencies installed successfully.
echo.

:: 5. Install Frontend Dependencies & Build Production Bundle
echo [5/6] Installing Frontend Next.js dependencies...
cd ..\frontend
call npm install
if %ERRORLEVEL% NEQ 0 (
    color 0C
    echo [ERROR] Failed to install frontend dependencies!
    pause
    exit /b 1
)
echo.
echo Building Next.js optimized frontend application...
call npm run build
if %ERRORLEVEL% NEQ 0 (
    color 0C
    echo [ERROR] Frontend build encountered errors!
    pause
    exit /b 1
)
cd ..\scripts
echo   - Frontend application built successfully.
echo.

:: 6. Environment Configurations
echo [6/6] Verifying environment files...
if not exist "..\backend\.env" (
    echo PORT=5000> "..\backend\.env"
    echo MQTT_URL=mqtt://127.0.0.1:1883>> "..\backend\.env"
    echo START_SIMULATOR=true>> "..\backend\.env"
    echo   - Created default ..\backend\.env
)

color 0A
echo.
echo ==============================================================================
echo [SUCCESS] INSTALLATION COMPLETE!
echo You can now launch the system by running: scripts\START_AMT.bat
echo ==============================================================================
echo.
pause

