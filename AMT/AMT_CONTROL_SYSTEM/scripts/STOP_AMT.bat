@echo off
title Stopping AMT Control System...
color 0C

echo ==============================================================================
echo       ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM
echo                          Master Shutdown Manager
echo ==============================================================================
echo.

echo Terminating running services gracefully...

:: Terminate Node.js processes associated with AMT
echo - Stopping Backend and Frontend Node.js processes...
taskkill /FI "WINDOWTITLE eq AMT Backend*" /T /F >nul 2>&1
taskkill /FI "WINDOWTITLE eq AMT Frontend*" /T /F >nul 2>&1

:: Stop Mosquitto Broker if running
echo - Stopping Mosquitto MQTT Broker...
taskkill /FI "WINDOWTITLE eq AMT Mosquitto*" /T /F >nul 2>&1
taskkill /IM mosquitto.exe /F >nul 2>&1

color 0A
echo.
echo ==============================================================================
echo [SHUTDOWN COMPLETE]
echo All AMT services stopped. Database records preserved safely.
echo ==============================================================================
echo.
timeout /t 3 >nul

