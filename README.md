# Hobby Projects in Embedded Systems

A curated collection of embedded systems, IoT, robotics, and hardware control projects.

---

## 📁 Projects in this Repository

### 1. [Automatic Manual Transmission (AMT) System](./AMT)
An industrial-grade, deterministic four-speed Automatic Manual Transmission control system designed for the **ESP32 microcontroller**:
- **Hardware Integration:** Optical IR RPM pulse counter, high-torque clutch servo (PCA9685 I2C), NEMA 17 stepper motor for gear shift, and torque-relief relay.
- **Dual-Tier Architecture:** High-speed real-time control on ESP32 paired with a PC supervisory dashboard.
- **IoT & Telemetry:** Next.js real-time cockpit, Node.js backend, Mosquitto MQTT broker, and SQLite telemetry logging.
- **Firmware Variants:** Includes modular PlatformIO C++ implementation and standalone Arduino IDE sketches.

---

## 🛠️ Technologies & Protocols
- **Hardware & Microcontrollers:** ESP32 (38-pin DevKit V1), Arduino, PCA9685, L298N, Optical Sensors
- **Embedded Languages:** C / C++ (PlatformIO / Arduino Core)
- **Supervisory Software:** TypeScript, React, Next.js, Node.js, Express, Tailwind CSS
- **Communication Protocols:** MQTT, WebSockets, HTTP REST, I2C, UART
