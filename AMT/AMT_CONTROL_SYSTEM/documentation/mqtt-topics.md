# MQTT Telemetry & Command Protocol Specification

## Topic Architecture Overview

All communication between the ESP32 microcontroller and the PC supervisory backend passes through Mosquitto on port 1883 with JSON payloads.

| Topic | Direction | QoS | Retained | Rate | Purpose |
|---|---|---|---|---|---|
| `amt/telemetry` | ESP32 → PC | 0 | No | 10 Hz | Comprehensive periodic telemetry packet |
| `amt/rpm` | ESP32 → PC | 0 | No | 10 Hz | Engine tachometer readings & pulse count |
| `amt/gear` | ESP32 → PC | 1 | Yes | On-change | Active gear position, target gear, and shifting state |
| `amt/servo` | ESP32 → PC | 0 | No | On-change | Clutch servo angular displacement & travel state |
| `amt/stepper` | ESP32 → PC | 0 | No | On-change | Stepper motor step coordinates & motion status |
| `amt/engine` | ESP32 → PC | 1 | Yes | On-change | Engine torque cut relay state |
| `amt/status` | ESP32 → PC | 1 | Yes | On-change | Device network connectivity, IP, RSSI, and Last Will |
| `amt/commands` | PC → ESP32 | 1 | No | Asynchronous | Real-time gear shift, mode, jog, and calibration commands |
| `amt/config` | Bidirectional | 1 | Yes | On-change | Live parameter adjustments & threshold updates |
| `amt/faults` | ESP32 → PC | 2 | Yes | On-event | Supervisor fault codes & diagnostic alarms |
| `amt/events` | ESP32 → PC | 1 | No | On-event | Shift completion audit records and command ACKs |
| `amt/emergency` | Bidirectional | 2 | Yes | On-event | Hardware & software emergency stop synchronization |
| `amt/heartbeat` | Bidirectional | 0 | No | 1 Hz | Watchdog liveliness ping & memory heap monitoring |

---

## Payload Schemas & Examples

### 1. `amt/telemetry`
Published at 100ms intervals (10 Hz).
```json
{
  "timestamp": 458921,
  "rpm": 3240.5,
  "avg_rpm": 3210.0,
  "max_rpm": 4820.0,
  "gear": 2,
  "target_gear": 2,
  "shift_state": 0,
  "mode": "AUTO",
  "clutch_angle": 15.0,
  "clutch_disengaged": false,
  "stepper_position": 1000,
  "stepper_moving": false,
  "engine_cut": false,
  "emergency": false,
  "speed_kmh": 32.4,
  "total_shifts": 14
}
```

### 2. `amt/commands`
Sent by PC to command actuator movements or shift requests.
- **Direct Gear Shift:**
  ```json
  { "cmd": "SHIFT", "gear": 3, "timestamp": 1731518290000 }
  ```
- **Paddle Shift:**
  ```json
  { "cmd": "UPSHIFT", "timestamp": 1731518290000 }
  ```
- **Mode Toggle:**
  ```json
  { "cmd": "SET_MODE", "mode": "AUTO", "timestamp": 1731518290000 }
  ```
- **Manual Clutch Command:**
  ```json
  { "cmd": "CLUTCH", "disengage": true, "timestamp": 1731518290000 }
  ```
- **Stepper Jog:**
  ```json
  { "cmd": "STEPPER_JOG", "steps": 50, "timestamp": 1731518290000 }
  ```
- **Calibrate Gear Step Position:**
  ```json
  { "cmd": "CALIBRATE_GEAR", "gear": 2, "position": 1000, "timestamp": 1731518290000 }
  ```

### 3. `amt/events`
Emitted by ESP32 upon completing a shift or acknowledging commands.
```json
{
  "event": "SHIFT_EVENT",
  "from_gear": 1,
  "to_gear": 2,
  "success": true,
  "duration_ms": 324,
  "rpm": 3200.0,
  "timestamp": 459245
}
```

### 4. `amt/emergency`
Latched state topic (QoS 2).
```json
{
  "emergency_active": true,
  "reason": "Hardware Emergency Stop button pressed!",
  "timestamp": 460120
}
```

### 5. `amt/heartbeat`
Published once every second (1 Hz).
```json
{
  "beat": 461000,
  "uptime_sec": 461,
  "free_heap": 184520,
  "wifi_rssi": -52
}
```

