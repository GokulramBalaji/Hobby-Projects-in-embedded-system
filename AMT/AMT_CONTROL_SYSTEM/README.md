# Advanced IoT Automatic Manual Transmission (AMT) System

An industrial-grade, deterministic, and modular four-speed Automatic Manual Transmission (AMT) control system engineered for the **ESP32 microcontroller**, paired with a PC-hosted supervisory application built on **Next.js, Node.js, Mosquitto MQTT, and SQLite**.

---

## 1. System Architecture & Features

- **Dual-Tier Control Architecture:**
  - **ESP32 Microcontroller:** Executes hard real-time actuator control, RPM pulse counting via hardware interrupts, 10-phase shifting state machine, and hardware failsafe supervisors.
  - **PC Supervisory Cockpit:** High-speed real-time dashboard, time-series telemetry charts, shift cycle audits, and parameter calibration.
- **Hardware Integration:**
  - **RPM Sensor:** V183 Optical IR sensor with moving average filter and noise glitch rejection.
  - **Clutch Actuator:** 25 kg·cm high-torque metal-gear digital servo driven via PCA9685 12-bit PWM controller over I2C.
  - **Gear-Shift Actuator:** NEMA 17 (423HT40-1204A) bipolar stepper motor with L298N driver and STEP/DIR driver support.
  - **Engine Torque Relief:** Optocoupled relay for ignition/fuel interruption during gear displacement.
  - **Emergency Stop:** Dedicated Normally Closed (NC) hardware failsafe interrupt.
- **Independent Communications:**
  - Standalone HTTP Web Server & WebSockets directly on ESP32 (`http://192.168.4.1` / `http://amt.local`).
  - High-throughput MQTT connection to PC supervisory broker on port 1883 across 13 distinct topics.
  - Integrated hardware simulation engine for bench-testing without physical motors attached.

---

## 2. Directory Structure

```
AMT_CONTROL_SYSTEM/
├── firmware/                  # PlatformIO C++ ESP32 Firmware
│   ├── include/               # Modular C++ Headers (Config, RPM, Servo, Stepper, Transmission, etc.)
│   ├── src/                   # C++ Implementation Modules
│   ├── platformio.ini         # PlatformIO Environment & Dependency Configuration
│   └── min_spiffs.csv         # Custom Flash Partition Table
│
├── backend/                   # Node.js Supervisory Backend
│   ├── src/
│   │   ├── controllers/       # REST API Endpoints (Control, Telemetry, Analytics, Config, CSV Export)
│   │   ├── mqtt/              # MQTT Client & Embedded Aedes Broker Fallback
│   │   ├── database/          # SQLite WAL Database & Schema
│   │   ├── simulation/        # Realistic Engine Kinetics & Shift Dynamics Simulator
│   │   └── index.js           # Express & WebSockets Master Server
│   └── package.json
│
├── frontend/                  # Next.js Automotive Instrument Dashboard
│   ├── app/                   # App Router Layout & Master Cockpit Page
│   ├── components/            # UI Components (Gauges, Indicators, Status Cards, Pipelines)
│   │   └── views/             # 12 Specialized Engineering Views
│   ├── hooks/                 # Real-time WebSocket Telemetry Hooks
│   ├── services/              # REST API Client
│   └── package.json
│
├── mqtt/                      # Mosquitto Broker Configuration
│   └── mosquitto.conf
│
├── scripts/                   # Automated Windows 10/11 Batch Scripts
│   ├── INSTALL_AMT.bat        # Automated Dependency & Database Installer
│   ├── START_AMT.bat          # Master Startup Manager
│   └── STOP_AMT.bat           # Graceful Shutdown Script
│
├── config/
│   └── default-config.json    # Factory System & Hardware Calibration Presets
│
├── documentation/             # Comprehensive Engineering Manuals
│   ├── wiring.md              # Pinout, Level Shifting, & Electrical Safety
│   ├── architecture.md        # State Machine & Architecture Flowcharts
│   ├── mqtt-topics.md         # Topic Dictionary & JSON Schemas
│   ├── installation.md        # Windows Setup & Firmware Flashing
│   ├── calibration.md         # Actuator Throw & RPM Sensor Calibration
│   └── troubleshooting.md     # Diagnostic Flowcharts & Fault Resolutions
│
├── INSTALL_AMT.bat            # Root Quick Installer
├── START_AMT.bat              # Root Quick Start
├── STOP_AMT.bat               # Root Quick Stop
├── .env.example
├── .gitignore
└── README.md
```

---

## 3. Quick Start (Windows 10 / 11)

### 1. Installation
Double-click `INSTALL_AMT.bat` from File Explorer or run in terminal:
```cmd
INSTALL_AMT.bat
```

### 2. Flashing ESP32 Firmware
Open `AMT_CONTROL_SYSTEM/firmware` in VS Code with PlatformIO, connect the ESP32 via USB, and click **Upload**.

### 3. Launch System
Double-click `START_AMT.bat`. Your web browser will automatically open:
```
http://localhost:3000
```

To run without physical hardware connected, toggle the **"SIMULATOR: ON"** switch in the top cockpit navigation bar!

### 4. Safe Shutdown
Double-click `STOP_AMT.bat` to gracefully stop all services.

---

## 4. Hardware Pinout Reference (ESP32-S 38-Pin)

| Component | ESP32 Pin | Signal Type | Electrical Requirement |
|---|---|---|---|
| **V183 IR Sensor** | GPIO 34 | Input (Interrupt) | 3.3V Logic (1kΩ/2kΩ divider if 5V) |
| **Emergency Stop** | GPIO 35 | Input (Interrupt) | Normally Closed (NC) loop to GND |
| **PCA9685 SDA** | GPIO 21 | I2C Data | 3.3V Logic |
| **PCA9685 SCL** | GPIO 22 | I2C Clock | 3.3V Logic |
| **L298N IN1** | GPIO 25 | Output | Coil A+ (or STEP in Step/Dir) |
| **L298N IN2** | GPIO 26 | Output | Coil A- (or DIR in Step/Dir) |
| **L298N IN3** | GPIO 27 | Output | Coil B+ (or ENABLE in Step/Dir) |
| **L298N IN4** | GPIO 14 | Output | Coil B- |
| **Engine Cut Relay** | GPIO 19 | Output | Optocoupled Active LOW/HIGH |
| **Status LED** | GPIO 2 | Output | Blue Onboard LED |

