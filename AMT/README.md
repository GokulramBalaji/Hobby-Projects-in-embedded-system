# Automatic Manual Transmission (AMT) - ESP32 Embedded System

An industrial-grade, deterministic, and modular four-speed Automatic Manual Transmission (AMT) control system engineered for the **ESP32 microcontroller**, featuring both standalone firmware implementations and an IoT supervisory dashboard.

---

## 📁 Repository Structure

```
AMT/
├── AMT_CONTROL_SYSTEM/                 # Full-stack IoT AMT System
│   ├── firmware/                       # Modular PlatformIO C++ ESP32 Firmware
│   ├── backend/                        # Node.js + Express + MQTT + SQLite supervisory server
│   ├── frontend/                       # Next.js + Tailwind CSS real-time telemetry cockpit
│   ├── documentation/                  # System architecture, wiring, and calibration docs
│   ├── mqtt/                           # Mosquitto broker configuration
│   └── scripts/                        # Automation & deployment scripts
│
├── AMT_ESP32_Firmware/                 # Standalone Arduino IDE firmware sketch
│   └── AMT_ESP32_Firmware.ino
│
├── ESP32_4Gear_AMT_RPM_Servo_Controller/ # 4-Gear closed-loop RPM & Servo controller
│   └── ESP32_4Gear_AMT_RPM_Servo_Controller.ino
│
├── esp32_amt_controller_claude.ino/    # Optimized single-file Arduino sketch
│   └── esp32_amt_controller_claude.ino
│
└── AMT_Connection_and_WiFi_Guide.pdf   # Complete hardware wiring & WiFi setup guide
```

---

## ⚙️ Key Hardware Components

- **Microcontroller:** ESP32 (38-pin DevKit V1)
- **RPM Sensor:** V183 Optical IR sensor with moving average filter and noise glitch rejection
- **Clutch Actuation:** 25 kg·cm high-torque metal-gear digital servo driven via PCA9685 12-bit PWM controller (I2C)
- **Gear-Shift Actuation:** NEMA 17 (423HT40-1204A) bipolar stepper motor with L298N / STEP-DIR driver
- **Torque Relief:** Optocoupled relay for ignition/fuel cut during shift transitions
- **Safety:** Hardware NC emergency stop interrupt & watchdog supervisor

---

## 🚀 Quick Start

### 1. Standalone Arduino Firmware
Open any `.ino` file in the Arduino IDE (e.g., `AMT_ESP32_Firmware/AMT_ESP32_Firmware.ino`), select **ESP32 Dev Module**, and upload.

### 2. Full IoT System (PlatformIO + Node.js + Next.js)
Navigate to `AMT_CONTROL_SYSTEM/` and run:
```cmd
scripts\INSTALL_AMT.bat
scripts\START_AMT.bat
```
Visit `http://localhost:3000` to access the real-time telemetry dashboard.
