# AMT Control System — Windows Installation & Flashing Guide

## 1. Prerequisites (Windows 10 / 11)

1. **Node.js:** Install Node.js v18.x or v20.x+ LTS from [https://nodejs.org](https://nodejs.org) (Includes NPM).
2. **CP2102 USB-to-UART Driver:** Download and install the Silicon Labs CP210x Universal Windows Driver so Windows recognizes the ESP32 COM port.
3. **PlatformIO IDE (or VS Code PlatformIO extension):** Install Visual Studio Code and add the **PlatformIO IDE** extension for 1-click compiling and firmware flashing.
4. **Mosquitto MQTT Broker (Optional):** Download from [https://mosquitto.org/download/](https://mosquitto.org/download/). (Note: The Node.js backend includes an automatic embedded Aedes broker on port 1883 if Mosquitto is not installed).

---

## 2. One-Click Automated Setup

Open a terminal or File Explorer, navigate to `AMT_CONTROL_SYSTEM/`, and execute:

```cmd
INSTALL_AMT.bat
```

This automated batch installer will:
1. Validate Node.js and NPM runtime environment.
2. Check Mosquitto MQTT Broker availability.
3. Install backend dependencies (`express`, `mqtt`, `sqlite3`, `ws`, `aedes`).
4. Install frontend dependencies (`next`, `react`, `chart.js`, `lucide-react`, `tailwindcss`).
5. Build the optimized production Next.js frontend application.
6. Initialize the SQLite relational database and seed factory default calibration parameters.

---

## 3. ESP32 Firmware Flashing via PlatformIO

1. Open **Visual Studio Code**.
2. Open the directory `AMT_CONTROL_SYSTEM/firmware`.
3. Connect the ESP32 (CP2102) to your computer using a USB data cable.
4. Verify COM port in Windows Device Manager (e.g. `COM3` or `COM4`).
5. In VS Code, click the PlatformIO alien icon on the left bar and click **Upload** (or run `pio run --target upload` in terminal).
6. PlatformIO will download the ESP32 Arduino framework, fetch the required libraries (`PubSubClient`, `Adafruit PWM Servo Driver`, `ArduinoJson`, `WebSockets`), compile, and flash the ESP32.

---

## 4. Starting the AMT Control System

Double-click:

```cmd
START_AMT.bat
```

This script will:
1. Start the Mosquitto MQTT broker on port 1883.
2. Start the Express API and WebSocket server on port 5000.
3. Start the Next.js Automotive Dashboard on port 3000.
4. Automatically open your default web browser to:
   ```
   http://localhost:3000
   ```

---

## 5. Shutting Down the System

To stop all background processes without corrupting SQLite records, double-click:

```cmd
STOP_AMT.bat
```

