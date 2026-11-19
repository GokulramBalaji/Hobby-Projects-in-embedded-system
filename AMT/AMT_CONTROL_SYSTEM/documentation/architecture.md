# System Architecture & Control Engineering Specification

## 1. Architectural Philosophy & Separation of Concerns

The AMT Control System operates under a strict dual-tier automotive architecture:

1. **Deterministic Embedded Core (ESP32):**
   - Retains 100% of real-time motor control, sensor interrupts, timing delays, and hardware failsafe responsibilities.
   - Operates completely autonomously. If Wi-Fi drops, MQTT disconnects, or the PC reboots, the ESP32 safety supervisor continues real-time shifting, watchdog monitoring, and emergency stop protection without interruption.

2. **Supervisory Analytics & Cockpit Tier (PC Local Host):**
   - High-level telemetry aggregation, time-series plotting, shift audit logging, parameter calibration, and predictive diagnostics.
   - Never directly pulses motor coils or blocks the microcontroller's hard real-time loop.

```mermaid
graph TD
    subgraph SENSORS_ACTUATORS["Physical Powertrain & Hardware"]
        IR[V183 IR Sensor] -->|Pulses| ESP32
        ESTOP[Hardware E-Stop Button] -->|NC Loop| ESP32
        ESP32 -->|I2C| PCA9685[PCA9685 PWM Driver]
        PCA9685 -->|PWM| SERVO[25 kg-cm Clutch Servo]
        ESP32 -->|4-Phase| L298N[L298N Driver]
        L298N -->|Bipolar Drive| STEPPER[NEMA 17 Stepper]
        ESP32 -->|GPIO 19| RELAY[Optocoupled Engine Relay]
    end

    subgraph ESP32_FIRMWARE["ESP32 Microcontroller Core"]
        ESP32[ESP32 System-on-Chip]
        RPM_MOD[RPM Module ISR]
        AMT_SM[10-Phase Shift State Machine]
        SAFETY_SUP[Safety Supervisor & WDT]
        ESP_WEB[Embedded WebServer & WS]
        MQTT_CLIENT[PubSubClient Gateway]
    end

    subgraph NETWORK["Wireless Transport"]
        ESP32 <-->|Wi-Fi / TCP-IP| BROKER[Mosquitto MQTT Broker]
        ESP32 <-->|Direct HTTP/WS| BROWSER_DIRECT[Direct Mobile/PC Browser]
    end

    subgraph PC_HOST["PC Supervisory System (Windows)"]
        BROKER <-->|MQTT.js| BACKEND[Node.js / Express Backend]
        BACKEND -->|ACID WAL| SQLITE[(SQLite Relational DB)]
        BACKEND <-->|WebSocket Stream| FRONTEND[Next.js Automotive Cockpit]
    end
```

---

## 2. Shift Sequencing State Machine (10-Phase Interlock)

To prevent gear tooth clash, transmission bind, or overlapping movements, the transmission executes a deterministic 10-phase sequence:

```mermaid
sequenceDiagram
    autonumber
    participant Driver as Operator / Auto Logic
    participant TM as Transmission FSM
    participant Servo as Clutch Servo
    participant Relay as Engine Relay
    participant Stepper as Stepper Motor

    Driver->>TM: Request Gear Shift (e.g. Gear 1 -> Gear 2)
    Note over TM: 1. Validate Target Gear & RPM Thresholds
    Note over TM: 2. Check Interlocks (Cooldown & Redline)
    TM->>Servo: 3. Disengage Clutch (Move to 95°)
    Note over Servo: Wait Calibrated Travel Delay (250ms)
    TM->>Relay: 4. Cut Engine Torque (Energize Relay)
    Note over Relay: Wait Pre-Cut Window (40ms)
    TM->>Stepper: 5. Command Step Presets to Target Gear (1000 steps)
    Note over Stepper: 6. Step Generation & Verify Arrival
    Stepper-->>TM: Target Position Reached
    TM->>Relay: 7. Restore Engine Torque (De-energize Relay)
    TM->>Servo: 8. Re-engage Clutch (Progressive 15° return)
    Note over TM: 9. Verify Final Gear Position & Delta RPM
    TM-->>Driver: 10. Shift Complete & Publish Event Log
```

---

## 3. SQLite Relational Database Architecture

```mermaid
erDiagram
    DEVICES ||--o{ SHIFT_EVENTS : executes
    DEVICES {
        string id PK
        string name
        string ip_address
        string status
        datetime last_seen
    }
    CONFIGURATIONS {
        int id PK
        string profile_name
        string config_json
        int is_active
        datetime created_at
    }
    RPM_READINGS {
        int id PK
        datetime timestamp
        float instantaneous_rpm
        float average_rpm
        float max_rpm
    }
    SHIFT_EVENTS {
        int id PK
        datetime timestamp
        int from_gear
        int to_gear
        int duration_ms
        int success
        float rpm
        string mode
    }
    FAULT_LOGS {
        int id PK
        datetime timestamp
        int fault_code
        string name
        string description
        int acknowledged
    }
```

