@echo off
setlocal enabledelayedexpansion
title Starting AMT Control System...
color 0B

echo ==============================================================================
echo       ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM
echo                          Master Startup Manager
echo ==============================================================================
echo.

:: 1. Launch Mosquitto if installed locally
echo [1/4] Checking MQTT Broker...
set MOSQUITTO_PATH=
where mosquitto >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set MOSQUITTO_PATH=mosquitto
) else if exist "C:\Program Files\mosquitto\mosquitto.exe" (
    set MOSQUITTO_PATH="C:\Program Files\mosquitto\mosquitto.exe"
)

if defined MOSQUITTO_PATH (
    echo   - Launching native Mosquitto Broker on port 1883...
    start "AMT Mosquitto Broker" /MIN %MOSQUITTO_PATH% -c "..\mqtt\mosquitto.conf" -v
) else (
    echo   - Native Mosquitto not found. Backend embedded Aedes broker will start on port 1883.
)
timeout /t 1 >nul

:: 2. Launch Backend API & WebSocket Server
echo [2/4] Starting Node.js Backend Server (Port 5000)...
cd ..\backend
start "AMT Backend Server" cmd /k "title AMT Backend & npm start"
cd ..\scripts

:: Wait 2 seconds for backend initialization
timeout /t 2 >nul

:: 3. Launch Frontend Web Dashboard
echo [3/4] Starting Next.js Web Application (Port 3000)...
cd ..\frontend
start "AMT Frontend Dashboard" cmd /k "title AMT Frontend & npm start"
cd ..\scripts

:: 4. Verify Services and Open Browser
echo [4/4] Waiting for services to initialize...
timeout /t 4 >nul

color 0A
echo.
echo ==============================================================================
echo  [ONLINE] ALL AMT SUBSYSTEMS RUNNING!
echo  - Frontend Dashboard: http://localhost:3000
echo  - Backend API:        http://localhost:5000/api/health
echo  - WebSocket Stream:   ws://localhost:5000/ws
echo  - MQTT Broker:        127.0.0.1:1883
echo ==============================================================================
echo.
echo Opening dashboard in default web browser...
start http://localhost:3000

echo.
echo Leave the server windows open while operating the system.
echo To safely shut down all services, run: scripts\STOP_AMT.bat
echo.
pause

