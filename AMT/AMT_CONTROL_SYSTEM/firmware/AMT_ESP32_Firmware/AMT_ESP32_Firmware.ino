/*
====================================================================================================
        ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM — ESP32 FIRMWARE
====================================================================================================
Target Microcontroller : ESP32 Dev Module (ESP32-S 38-Pin with CP2102)
Framework              : Arduino IDE / PlatformIO
Firmware Version       : 2.4.0

REQUIRED ARDUINO LIBRARIES (Install via Tools -> Manage Libraries):
  1. ArduinoJson by Benoit Blanchon (v6.21.x or v7.x)
  2. PubSubClient by Nick O'Leary (v2.8+)
  3. Adafruit PWM Servo Driver Library by Adafruit (v3.0.x)
  4. WebSockets by Markus Sattler (v2.4.x)

HARDWARE PIN DEFINITIONS (38-PIN ESP32S):
  - GPIO 34 : V183 Optical IR Sensor DO (via 1k/2k voltage divider if 5V)
  - GPIO 35 : Hardware Emergency Stop (Normally Closed to GND with 10k pull-up to 3.3V)
  - GPIO 21 : PCA9685 I2C SDA (3.3V Logic)
  - GPIO 22 : PCA9685 I2C SCL (3.3V Logic)
  - GPIO 25 : Stepper IN1 (Coil A+ / STEP)
  - GPIO 26 : Stepper IN2 (Coil A- / DIR)
  - GPIO 27 : Stepper IN3 (Coil B+ / ENABLE)
  - GPIO 14 : Stepper IN4 (Coil B-)
  - GPIO 19 : Engine Torque Cut Relay (Optocoupled)
  - GPIO 2  : Status LED (Onboard Blue)

SAFETY & ELECTRICAL NOTICE:
  - NEVER power the 25 kg·cm servo from ESP32 VIN or 3.3V! Connect PCA9685 V+ to an external
    6V 5A-10A switching power supply with common ground.
  - The L298N will heat up under continuous NEMA 17 current; firmware automatically de-energizes
    coils after 400ms idle.
====================================================================================================
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// =================================================================================================
// 1. PIN ALLOCATIONS & SYSTEM CONSTANTS
// =================================================================================================
#define PIN_RPM_SENSOR         34   // V183 IR Sensor (Input Only)
#define PIN_ESTOP_HW           35   // Hardware Emergency Stop (Normally Closed loop to GND)
#define PIN_I2C_SDA            21   // PCA9685 I2C SDA
#define PIN_I2C_SCL            22   // PCA9685 I2C SCL
#define PIN_STEPPER_IN1        25   // L298N Coil A+ (or STEP)
#define PIN_STEPPER_IN2        26   // L298N Coil A- (or DIR)
#define PIN_STEPPER_IN3        27   // L298N Coil B+ (or ENABLE)
#define PIN_STEPPER_IN4        14   // L298N Coil B-
#define PIN_ENGINE_RELAY       19   // Optocoupled Relay for ignition/fuel cut
#define PIN_STATUS_LED         2    // Onboard status indicator

#define AMT_FIRMWARE_VER       "2.4.0"
#define AMT_DEFAULT_WIFI_SSID  "AMT_WIFI_NETWORK"
#define AMT_DEFAULT_WIFI_PASS  "amt_secure_password"
#define AMT_DEFAULT_AP_SSID    "AMT-CONTROLLER-AP"
#define AMT_DEFAULT_AP_PASS    "amtadminpassword"
#define AMT_DEFAULT_MQTT_HOST  "192.168.1.100"
#define AMT_DEFAULT_MQTT_PORT  1883

// Transmission Operating Modes
enum TransmissionMode {
    MODE_MANUAL = 0,
    MODE_AUTOMATIC = 1
};

// 10-Phase Deterministic Shift State Machine
enum ShiftState {
    SHIFT_IDLE = 0,
    SHIFT_VALIDATING = 1,
    SHIFT_CLUTCH_DISENGAGING = 2,
    SHIFT_ENGINE_CUTTING = 3,
    SHIFT_STEPPER_MOVING = 4,
    SHIFT_VERIFYING_STEPPER = 5,
    SHIFT_ENGINE_RESTORING = 6,
    SHIFT_CLUTCH_ENGAGING = 7,
    SHIFT_VERIFYING_GEAR = 8,
    SHIFT_COMPLETED = 9,
    SHIFT_FAILED = 10,
    SHIFT_EMERGENCY_ABORT = 11
};

// System Fault Codes
enum FaultCode {
    FAULT_NONE = 0,
    FAULT_SENSOR_DISCONNECTED = 1,
    FAULT_IMPLAUSIBLE_RPM = 2,
    FAULT_STEPPER_TIMEOUT = 3,
    FAULT_SERVO_DRIVER_ERROR = 4,
    FAULT_ESTOP_ACTIVE = 5,
    FAULT_WATCHDOG_TIMEOUT = 6,
    FAULT_SHIFT_OVERLAP_BLOCKED = 7,
    FAULT_MQTT_DISCONNECTED = 8
};

// =================================================================================================
// 2. CONFIGURATION DATA STRUCTURE
// =================================================================================================
struct AMTConfig {
    uint16_t pulsesPerRevolution;
    uint32_t glitchFilterMicros;
    uint32_t sensorTimeoutMs;
    uint32_t maxPlausibleRpm;
    float    rpmCalibrationFactor;

    uint8_t  clutchServoChannel;
    uint16_t clutchEngagedAngle;
    uint16_t clutchDisengagedAngle;
    uint32_t clutchTravelTimeMs;

    uint16_t maxSpeedStepsPerSec;
    int32_t  gearPositions[5]; // 0=Neutral, 1=G1, 2=G2, 3=G3, 4=G4
    uint32_t stepperTimeoutMs;
    bool     useStepDirMode;

    bool     engineRelayActiveHigh;
    uint32_t engineCutDurationMs;

    uint32_t clutchPullDelayMs;
    uint32_t engineCutPreDelayMs;
    uint32_t engineRestoreDelayMs;
    uint32_t clutchReleaseDelayMs;
    uint32_t minShiftIntervalMs;

    struct {
        uint32_t upshiftRpm;
        uint32_t downshiftRpm;
        float    nominalSpeedKmh;
    } autoThresholds[5];
    uint32_t hysteresisRpm;
    uint32_t minEngineRpm;
    uint32_t redlineRpm;

    char     wifiSsid[33];
    char     wifiPassword[65];
    char     apSsid[33];
    char     apPassword[65];
    char     mqttBroker[65];
    uint16_t mqttPort;
    char     mqttUser[33];
    char     mqttPass[33];
    char     mqttClientId[33];

    bool     simulationMode;
    bool     estopNormallyClosed;
};

AMTConfig config;
Preferences preferences;

// =================================================================================================
// 3. HARDWARE CONTROLLERS & OBJECTS
// =================================================================================================
WiFiClient              wifiClient;
PubSubClient            mqttClient(wifiClient);
WebServer               httpServer(80);
WebSocketsServer        wsServer(81);
Adafruit_PWMServoDriver pca9685(0x40);

// Stepper Half-Step Sequence Table (400 steps/rev on 1.8° NEMA 17)
static const uint8_t L298N_HALF_STEPS[8][4] = {
    {1, 0, 0, 0}, {1, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0},
    {0, 1, 0, 0}, {0, 1, 0, 1}, {0, 0, 0, 1}, {1, 0, 0, 1}
};

// Embedded Web Dashboard HTML stored in Flash PROGMEM
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 AMT Standalone Controller</title>
    <style>
        :root { --bg: #090d16; --card: #111827; --accent: #2563eb; --text: #f8fafc; --text-muted: #94a3b8; --danger: #ef4444; --success: #10b981; --border: #1f2937; }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
        .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--border); padding-bottom: 12px; margin-bottom: 16px; }
        .title { font-size: 1.15rem; font-weight: 800; letter-spacing: 0.5px; }
        .pill { padding: 4px 10px; border-radius: 999px; font-size: 0.75rem; font-weight: 700; background: #1e293b; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 14px; margin-bottom: 16px; }
        .card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 16px; display: flex; flex-direction: column; }
        .card-title { font-size: 0.75rem; text-transform: uppercase; color: var(--text-muted); font-weight: 700; margin-bottom: 6px; }
        .big-val { font-size: 3rem; font-weight: 900; line-height: 1; font-family: monospace; color: #fff; }
        .gear-badge { font-size: 4.5rem; font-weight: 900; color: #38bdf8; text-align: center; margin: 8px 0; font-family: monospace; }
        .controls { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; margin-top: 10px; }
        button { background: #1e293b; color: #fff; border: 1px solid #334155; padding: 12px; border-radius: 10px; font-weight: 700; font-size: 0.9rem; cursor: pointer; transition: 0.15s; }
        button:hover { background: #334155; }
        button.primary { background: var(--accent); border-color: #3b82f6; }
        button.primary:hover { background: #1d4ed8; }
        button.danger { background: var(--danger); border-color: #f87171; width: 100%; padding: 16px; font-size: 1.1rem; }
        button.danger:hover { background: #dc2626; }
        .row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #1f2937; font-size: 0.85rem; }
        .dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; background: var(--success); margin-right: 6px; }
    </style>
</head>
<body>
    <div class="header">
        <div class="title">⚡ ESP32 AMT STANDALONE CONTROLLER</div>
        <div id="connStat" class="pill"><span class="dot"></span>CONNECTING...</div>
    </div>
    <div class="grid">
        <div class="card">
            <div class="card-title">Engine Rotational Speed</div>
            <div class="big-val"><span id="rpmVal">0</span> <span style="font-size:0.9rem; color:#94a3b8;">RPM</span></div>
            <div class="row" style="margin-top:auto;"><span>Average RPM:</span><b id="avgRpm">0</b></div>
            <div class="row"><span>Peak RPM:</span><b id="maxRpm">0</b></div>
        </div>
        <div class="card" style="text-align: center;">
            <div class="card-title">Active Transmission Gear</div>
            <div id="gearVal" class="gear-badge">N</div>
            <div id="shiftState" style="font-size:0.8rem; color:#94a3b8; font-weight:700;">STANDBY</div>
        </div>
        <div class="card">
            <div class="card-title">Transmission Cockpit Controls</div>
            <div class="controls">
                <button class="primary" onclick="sendCmd('UP')">▲ UPSHIFT</button>
                <button class="primary" onclick="sendCmd('DOWN')">▼ DOWNSHIFT</button>
                <button onclick="sendGear(0)">NEUTRAL (N)</button>
                <button onclick="toggleMode()" id="modeBtn">MODE: MANUAL</button>
            </div>
        </div>
        <div class="card">
            <div class="card-title">Actuator Diagnostics</div>
            <div class="row"><span>Clutch Servo:</span><b id="clutchStat">Engaged (15°)</b></div>
            <div class="row"><span>Stepper Position:</span><b id="stepperStat">0 Steps</b></div>
            <div class="row"><span>Torque Interruption:</span><b id="engineStat">Normal (Active)</b></div>
            <div class="row"><span>Total Shifts:</span><b id="shiftsCount">0</b></div>
        </div>
    </div>
    <button class="danger" onclick="toggleEstop()" id="estopBtn">🚨 EMERGENCY STOP</button>
    <script>
        let ws, isAuto = false, isEstop = false;
        function connectWs() {
            ws = new WebSocket("ws://" + (window.location.hostname || "192.168.4.1") + ":81/");
            ws.onopen = () => { document.getElementById("connStat").innerHTML = '<span class="dot" style="background:#10b981"></span>ONLINE (WS)'; };
            ws.onclose = () => { document.getElementById("connStat").innerHTML = '<span class="dot" style="background:#ef4444"></span>OFFLINE'; setTimeout(connectWs, 2000); };
            ws.onmessage = (e) => {
                try {
                    const d = JSON.parse(e.data);
                    document.getElementById("rpmVal").innerText = Math.round(d.rpm);
                    document.getElementById("avgRpm").innerText = Math.round(d.avg_rpm);
                    document.getElementById("maxRpm").innerText = Math.round(d.max_rpm);
                    const gears = ["N", "1", "2", "3", "4"];
                    document.getElementById("gearVal").innerText = gears[d.gear] || "?";
                    document.getElementById("shiftState").innerText = d.shifting ? "SHIFTING IN PROGRESS..." : "STANDBY";
                    document.getElementById("clutchStat").innerText = d.clutch_disengaged ? "Disengaged (95°)" : "Engaged (15°)";
                    document.getElementById("stepperStat").innerText = d.stepper_pos + " Steps";
                    document.getElementById("engineStat").innerText = d.engine_cut ? "TORQUE CUT (90ms)" : "Active Drive";
                    document.getElementById("shiftsCount").innerText = d.total_shifts;
                    isAuto = (d.mode === 1);
                    document.getElementById("modeBtn").innerText = isAuto ? "MODE: AUTO" : "MODE: MANUAL";
                    isEstop = d.estop;
                    const btn = document.getElementById("estopBtn");
                    btn.innerText = isEstop ? "⚠️ CLEAR EMERGENCY STOP" : "🚨 EMERGENCY STOP";
                    btn.style.background = isEstop ? "#d97706" : "#ef4444";
                } catch(err) {}
            };
        }
        function sendCmd(act) { fetch("/api/control/gear", { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({ action:act }) }); }
        function sendGear(g) { fetch("/api/control/gear", { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({ gear:g }) }); }
        function toggleMode() { fetch("/api/control/mode", { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({ mode: isAuto ? 0 : 1 }) }); }
        function toggleEstop() { fetch("/api/control/estop", { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify({ active: !isEstop }) }); }
        window.onload = connectWs;
    </script>
</body>
</html>
)rawliteral";

// =================================================================================================
// 4. RUNTIME SYSTEM VARIABLES
// =================================================================================================
// RPM Sensor runtime
volatile uint32_t s_pulseCount = 0;
volatile uint32_t s_lastPulseMicros = 0;
volatile uint32_t s_pulseIntervalMicros = 0;
float    gInstantaneousRpm = 0.0f;
float    gAverageRpm = 0.0f;
float    gMaxRpm = 0.0f;
bool     gSensorConnected = false;
bool     gSensorImplausible = false;
uint32_t gLastPulseTimeMs = 0;
#define AVG_SAMPLES 8
float    gRpmHistory[AVG_SAMPLES] = {0};
uint8_t  gRpmHistoryIdx = 0;

// Transmission runtime
uint8_t  gCurrentGear = 0;
uint8_t  gTargetGear = 0;
TransmissionMode gMode = MODE_MANUAL;
ShiftState gShiftState = SHIFT_IDLE;
uint32_t gShiftStartTimeMs = 0;
uint32_t gStateStartTimeMs = 0;
uint32_t gLastShiftCompleteTimeMs = 0;
uint32_t gLastShiftDurationMs = 0;
uint32_t gTotalShifts = 0;
uint32_t gSuccessfulShifts = 0;
uint32_t gFailedShifts = 0;

// Actuator states
float    gClutchAngle = 15.0f;
bool     gClutchDisengaged = false;
int32_t  gStepperCurrentPosition = 0;
int32_t  gStepperTargetPosition = 0;
bool     gStepperMoving = false;
bool     gStepperStalled = false;
uint8_t  gStepperPhase = 0;
uint32_t gLastStepMicros = 0;
uint32_t gStepIntervalMicros = 1250;
uint32_t gStepperMoveStartMs = 0;
uint32_t gLastMoveCompletedMs = 0;
bool     gCoilsEnergized = false;

// Engine cut relay runtime
bool     gTorqueCut = false;
uint32_t gCutStartTimeMs = 0;
uint32_t gCutDurationMs = 0;

// Safety supervisor runtime
bool     gEmergencyActive = false;
FaultCode gLastFaultCode = FAULT_NONE;
char     gFaultMessage[64] = {0};
uint32_t gLastPcHeartbeatMs = 0;
uint32_t gEstopDebounceMs = 0;

// Network & MQTT runtime
bool     gIsApMode = false;
uint32_t gLastMqttReconnectAttemptMs = 0;
uint32_t gLastTelemetryPublishMs = 0;
uint32_t gLastHeartbeatMs = 0;
uint32_t gLastWsBroadcastMs = 0;

// =================================================================================================
// 5. INTERRUPT SERVICE ROUTINES (ISR)
// =================================================================================================
void IRAM_ATTR rpmSensorIsr() {
    uint32_t now = micros();
    uint32_t interval = now - s_lastPulseMicros;
    if (interval >= config.glitchFilterMicros) {
        s_pulseIntervalMicros = interval;
        s_lastPulseMicros = now;
        s_pulseCount++;
    }
}

// =================================================================================================
// 6. LOW-LEVEL ACTUATOR DRIVER FUNCTIONS
// =================================================================================================
void setEngineRelay(bool cutIgnition) {
    bool pinLevel;
    if (config.engineRelayActiveHigh) {
        pinLevel = cutIgnition ? HIGH : LOW;
    } else {
        pinLevel = cutIgnition ? LOW : HIGH;
    }
    digitalWrite(PIN_ENGINE_RELAY, pinLevel);
}

void cutTorque(uint32_t durationMs) {
    if (gEmergencyActive) return;
    gTorqueCut = true;
    gCutStartTimeMs = millis();
    gCutDurationMs = durationMs;
    setEngineRelay(true);
}

void restoreTorque() {
    if (gEmergencyActive) return;
    gTorqueCut = false;
    setEngineRelay(false);
}

void setClutchServoAngle(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    gClutchAngle = angle;
    gClutchDisengaged = (angle > 50.0f);

    // Map 0-180 deg to 500-2500 us, and convert to 12-bit ticks at 50Hz (20,000 us period)
    uint16_t pulseUs = 500 + (uint16_t)((angle / 180.0f) * 2000.0f);
    uint16_t ticks = (uint16_t)(((uint32_t)pulseUs * 4096UL) / 20000UL);
    pca9685.setPWM(config.clutchServoChannel, 0, ticks);
}

void disengageClutch() {
    setClutchServoAngle(config.clutchDisengagedAngle);
}

void engageClutch() {
    setClutchServoAngle(config.clutchEngagedAngle);
}

void enableStepperCoils(bool enable) {
    gCoilsEnergized = enable;
    if (!enable) {
        digitalWrite(PIN_STEPPER_IN1, LOW);
        digitalWrite(PIN_STEPPER_IN2, LOW);
        digitalWrite(PIN_STEPPER_IN3, LOW);
        digitalWrite(PIN_STEPPER_IN4, LOW);
    }
}

void applyStepperPhase(uint8_t phase) {
    if (phase >= 8) {
        enableStepperCoils(false);
        return;
    }
    digitalWrite(PIN_STEPPER_IN1, L298N_HALF_STEPS[phase][0] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN2, L298N_HALF_STEPS[phase][1] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN3, L298N_HALF_STEPS[phase][2] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN4, L298N_HALF_STEPS[phase][3] ? HIGH : LOW);
}

void moveStepperToPosition(int32_t targetStep) {
    if (gStepperCurrentPosition == targetStep && gStepperTargetPosition == targetStep) {
        return;
    }
    gStepperTargetPosition = targetStep;
    gStepperMoveStartMs = millis();
    gStepperStalled = false;
    gStepperMoving = true;
    enableStepperCoils(true);
}

void stopStepperImmediately() {
    gStepperTargetPosition = gStepperCurrentPosition;
    gStepperMoving = false;
    enableStepperCoils(false);
}

// =================================================================================================
// 7. SAFETY SUPERVISOR & EMERGENCY STOP
// =================================================================================================
void triggerEmergencyStop(FaultCode reason, const char* msg) {
    gEmergencyActive = true;
    gLastFaultCode = reason;
    if (msg) strncpy(gFaultMessage, msg, sizeof(gFaultMessage) - 1);

    // Immediate Failsafe Execution:
    setEngineRelay(true);               // 1. Permanently cut engine torque
    stopStepperImmediately();           // 2. Halt stepper motion & cut coils
    disengageClutch();                  // 3. Move clutch to disengaged safety angle
    gShiftState = SHIFT_EMERGENCY_ABORT;

    Serial.printf("[SAFETY] *** CRITICAL EMERGENCY STOP ACTIVATED ***: %s\n", gFaultMessage);

    // Notify MQTT
    if (mqttClient.connected()) {
        StaticJsonDocument<256> doc;
        doc["emergency_active"] = true;
        doc["reason"] = gFaultMessage;
        doc["code"] = (int)reason;
        char buf[256];
        serializeJson(doc, buf);
        mqttClient.publish("amt/emergency", buf, true);
        mqttClient.publish("amt/faults", buf);
    }
}

bool clearEmergencyStop() {
    bool pinState = digitalRead(PIN_ESTOP_HW);
    bool isPressed = config.estopNormallyClosed ? (pinState == HIGH) : (pinState == LOW);
    if (isPressed) {
        Serial.println("[SAFETY] Cannot clear: Hardware E-Stop button remains physically active!");
        return false;
    }

    gEmergencyActive = false;
    gLastFaultCode = FAULT_NONE;
    gFaultMessage[0] = '\0';

    // Safely restore nominal standby
    setEngineRelay(false);
    engageClutch();
    digitalWrite(PIN_STATUS_LED, HIGH);

    Serial.println("[SAFETY] Emergency Stop cleared. System returned to standby.");
    if (mqttClient.connected()) {
        mqttClient.publish("amt/emergency", "{\"emergency_active\":false,\"reason\":\"Cleared by operator\"}", true);
    }
    return true;
}

// =================================================================================================
// 8. TRANSMISSION STATE MACHINE & SHIFT SEQUENCING
// =================================================================================================
float estimateVehicleSpeedKmh(uint8_t gear, float rpm) {
    if (gear == 0 || rpm < 200.0f) return 0.0f;
    const float factors[5] = { 0.0f, 5.0f, 10.0f, 16.0f, 22.0f };
    return (rpm / 1000.0f) * factors[gear];
}

bool requestGearShift(uint8_t targetGear) {
    if (targetGear > 4 || gEmergencyActive) return false;
    if (gShiftState != SHIFT_IDLE) {
        Serial.println("[TRANSMISSION] Shift rejected: Cycle already in progress.");
        return false;
    }
    if (targetGear == gCurrentGear) return false;

    // Shift interval lockout
    if ((millis() - gLastShiftCompleteTimeMs) < config.minShiftIntervalMs) {
        Serial.println("[TRANSMISSION] Shift rejected: Minimum shift interval active.");
        return false;
    }

    // Downshift redline protection
    if (targetGear < gCurrentGear && targetGear > 0) {
        float ratios[5] = { 0.0f, 3.4f, 2.1f, 1.4f, 1.0f };
        float projectedRpm = gAverageRpm * (ratios[targetGear] / ratios[gCurrentGear]);
        if (projectedRpm > config.redlineRpm) {
            Serial.printf("[TRANSMISSION] Downshift rejected: Projected RPM %.0f exceeds redline %u!\n",
                          projectedRpm, config.redlineRpm);
            return false;
        }
    }

    gTargetGear = targetGear;
    gShiftStartTimeMs = millis();
    gStateStartTimeMs = millis();
    gShiftState = SHIFT_VALIDATING;
    return true;
}

void updateTransmissionStateMachine() {
    uint32_t nowMs = millis();

    if (gEmergencyActive) {
        if (gShiftState != SHIFT_EMERGENCY_ABORT && gShiftState != SHIFT_IDLE) {
            gShiftState = SHIFT_EMERGENCY_ABORT;
        }
        return;
    }

    // Evaluate automatic shifting when idle
    if (gMode == MODE_AUTOMATIC && gShiftState == SHIFT_IDLE) {
        if ((nowMs - gLastShiftCompleteTimeMs) >= config.minShiftIntervalMs && gCurrentGear > 0) {
            // Upshift evaluation
            if (gCurrentGear < 4 && gAverageRpm >= config.autoThresholds[gCurrentGear].upshiftRpm) {
                requestGearShift(gCurrentGear + 1);
            }
            // Downshift evaluation with hysteresis
            else if (gCurrentGear > 1 && gAverageRpm <= (config.autoThresholds[gCurrentGear].downshiftRpm - config.hysteresisRpm)) {
                requestGearShift(gCurrentGear - 1);
            }
        }
    }

    // 10-Phase Sequence Execution
    switch (gShiftState) {
        case SHIFT_IDLE:
            break;

        case SHIFT_VALIDATING:
            gShiftState = SHIFT_CLUTCH_DISENGAGING;
            gStateStartTimeMs = nowMs;
            disengageClutch(); // Step 3: Pull clutch
            break;

        case SHIFT_CLUTCH_DISENGAGING:
            if ((nowMs - gStateStartTimeMs) >= config.clutchPullDelayMs) {
                gShiftState = SHIFT_ENGINE_CUTTING;
                gStateStartTimeMs = nowMs;
                cutTorque(config.engineCutDurationMs); // Step 4: Cut engine torque
            }
            break;

        case SHIFT_ENGINE_CUTTING:
            if ((nowMs - gStateStartTimeMs) >= config.engineCutPreDelayMs) {
                gShiftState = SHIFT_STEPPER_MOVING;
                gStateStartTimeMs = nowMs;
                moveStepperToPosition(config.gearPositions[gTargetGear]); // Step 5: Move actuator
            }
            break;

        case SHIFT_STEPPER_MOVING:
            gShiftState = SHIFT_VERIFYING_STEPPER;
            break;

        case SHIFT_VERIFYING_STEPPER:
            if (gStepperStalled) {
                gShiftState = SHIFT_FAILED;
                triggerEmergencyStop(FAULT_STEPPER_TIMEOUT, "Stepper motor stalled during shift!");
                break;
            }
            if (!gStepperMoving) {
                gShiftState = SHIFT_ENGINE_RESTORING;
                gStateStartTimeMs = nowMs;
                restoreTorque(); // Step 7: Restore engine torque
            }
            break;

        case SHIFT_ENGINE_RESTORING:
            if ((nowMs - gStateStartTimeMs) >= config.engineRestoreDelayMs) {
                gShiftState = SHIFT_CLUTCH_ENGAGING;
                gStateStartTimeMs = nowMs;
                engageClutch(); // Step 8: Release clutch
            }
            break;

        case SHIFT_CLUTCH_ENGAGING:
            if ((nowMs - gStateStartTimeMs) >= config.clutchReleaseDelayMs) {
                gShiftState = SHIFT_VERIFYING_GEAR;
            }
            break;

        case SHIFT_VERIFYING_GEAR: {
            int32_t expected = config.gearPositions[gTargetGear];
            if (abs(gStepperCurrentPosition - expected) <= 25) {
                uint8_t oldGear = gCurrentGear;
                gCurrentGear = gTargetGear;
                gLastShiftDurationMs = millis() - gShiftStartTimeMs;
                gSuccessfulShifts++;
                gTotalShifts++;
                gLastShiftCompleteTimeMs = millis();
                gShiftState = SHIFT_COMPLETED;

                Serial.printf("[TRANSMISSION] Shift SUCCESS: G%d -> G%d in %u ms.\n",
                              oldGear, gCurrentGear, gLastShiftDurationMs);

                // Publish shift event to MQTT
                if (mqttClient.connected()) {
                    StaticJsonDocument<256> doc;
                    doc["event"] = "SHIFT_EVENT";
                    doc["from_gear"] = oldGear;
                    doc["to_gear"] = gCurrentGear;
                    doc["success"] = true;
                    doc["duration_ms"] = gLastShiftDurationMs;
                    doc["rpm"] = gAverageRpm;
                    doc["timestamp"] = millis();
                    char buf[256];
                    serializeJson(doc, buf);
                    mqttClient.publish("amt/events", buf);
                }
            } else {
                gShiftState = SHIFT_FAILED;
                triggerEmergencyStop(FAULT_STEPPER_TIMEOUT, "Gear verification failed: step mismatch!");
            }
            break;
        }

        case SHIFT_COMPLETED:
            gShiftState = SHIFT_IDLE;
            break;

        case SHIFT_FAILED:
        case SHIFT_EMERGENCY_ABORT:
            restoreTorque();
            engageClutch();
            gFailedShifts++;
            gTotalShifts++;
            gLastShiftCompleteTimeMs = millis();
            gShiftState = SHIFT_IDLE;
            break;
    }
}

// =================================================================================================
// 9. MQTT PROTOCOL & COMMAND PARSER
// =================================================================================================
void onMqttMessageReceived(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, message);
    if (err) return;

    if (strcmp(topic, "amt/commands") == 0) {
        const char* cmd = doc["cmd"] | "";
        bool ack = false;

        if (strcmp(cmd, "SHIFT") == 0) {
            uint8_t g = doc["gear"] | 0;
            ack = requestGearShift(g);
        } else if (strcmp(cmd, "UPSHIFT") == 0) {
            ack = (gCurrentGear < 4) ? requestGearShift(gCurrentGear + 1) : false;
        } else if (strcmp(cmd, "DOWNSHIFT") == 0) {
            ack = (gCurrentGear > 0) ? requestGearShift(gCurrentGear - 1) : false;
        } else if (strcmp(cmd, "NEUTRAL") == 0) {
            ack = requestGearShift(0);
        } else if (strcmp(cmd, "SET_MODE") == 0) {
            const char* m = doc["mode"] | "MANUAL";
            gMode = (strcmp(m, "AUTO") == 0) ? MODE_AUTOMATIC : MODE_MANUAL;
            ack = true;
        } else if (strcmp(cmd, "CLUTCH") == 0) {
            bool dis = doc["disengage"] | false;
            if (dis) disengageClutch(); else engageClutch();
            ack = true;
        } else if (strcmp(cmd, "STEPPER_JOG") == 0) {
            int32_t steps = doc["steps"] | 0;
            moveStepperToPosition(gStepperCurrentPosition + steps);
            ack = true;
        } else if (strcmp(cmd, "STEPPER_HOME") == 0) {
            moveStepperToPosition(0);
            ack = true;
        } else if (strcmp(cmd, "ESTOP") == 0) {
            bool active = doc["active"] | true;
            if (active) triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "MQTT Command E-Stop");
            else clearEmergencyStop();
            ack = true;
        } else if (strcmp(cmd, "RESET") == 0) {
            delay(200);
            ESP.restart();
        }

        // Emit command acknowledgement
        StaticJsonDocument<128> ackDoc;
        ackDoc["event"] = "CMD_ACK";
        ackDoc["cmd"] = cmd;
        ackDoc["status"] = ack ? "ACCEPTED" : "REJECTED";
        char buf[128];
        serializeJson(ackDoc, buf);
        mqttClient.publish("amt/events", buf);
    } else if (strcmp(topic, "amt/emergency") == 0) {
        bool act = doc["emergency_active"] | false;
        if (act) triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "MQTT Emergency Broadcast");
        else clearEmergencyStop();
    } else if (strcmp(topic, "amt/heartbeat") == 0) {
        gLastPcHeartbeatMs = millis();
    }
}

void connectMqttBroker() {
    Serial.printf("[MQTT] Connecting to broker %s:%d...\n", config.mqttBroker, config.mqttPort);
    const char* willTopic = "amt/status";
    const char* willPayload = "{\"status\":\"offline\",\"device\":\"ESP32_AMT\"}";

    bool ok = false;
    if (strlen(config.mqttUser) > 0) {
        ok = mqttClient.connect(config.mqttClientId, config.mqttUser, config.mqttPass, willTopic, 1, true, willPayload);
    } else {
        ok = mqttClient.connect(config.mqttClientId, willTopic, 1, true, willPayload);
    }

    if (ok) {
        Serial.println("[MQTT] Connected successfully.");
        mqttClient.subscribe("amt/commands");
        mqttClient.subscribe("amt/config");
        mqttClient.subscribe("amt/emergency");
        mqttClient.subscribe("amt/heartbeat");

        StaticJsonDocument<256> doc;
        doc["status"] = "online";
        doc["device"] = config.mqttClientId;
        doc["version"] = AMT_FIRMWARE_VER;
        doc["ip"] = gIsApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        char buf[256];
        serializeJson(doc, buf);
        mqttClient.publish("amt/status", buf, true);
    }
}

void publishTelemetryPacket() {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();
    doc["rpm"] = round(gInstantaneousRpm * 10) / 10.0;
    doc["avg_rpm"] = round(gAverageRpm * 10) / 10.0;
    doc["max_rpm"] = round(gMaxRpm * 10) / 10.0;
    doc["gear"] = gCurrentGear;
    doc["target_gear"] = gTargetGear;
    doc["shift_state"] = (int)gShiftState;
    doc["mode"] = (gMode == MODE_AUTOMATIC) ? "AUTO" : "MANUAL";
    doc["clutch_angle"] = gClutchAngle;
    doc["clutch_disengaged"] = gClutchDisengaged;
    doc["stepper_position"] = gStepperCurrentPosition;
    doc["stepper_moving"] = gStepperMoving;
    doc["engine_cut"] = gTorqueCut;
    doc["emergency"] = gEmergencyActive;
    doc["speed_kmh"] = round(estimateVehicleSpeedKmh(gCurrentGear, gAverageRpm) * 10) / 10.0;
    doc["total_shifts"] = gTotalShifts;

    char buf[512];
    serializeJson(doc, buf);
    mqttClient.publish("amt/telemetry", buf);
}

// =================================================================================================
// 10. REST API & WEBSOCKET BROADCAST
// =================================================================================================
void registerRestRoutes() {
    httpServer.on("/", HTTP_GET, []() {
        httpServer.send_P(200, "text/html", INDEX_HTML);
    });

    httpServer.on("/api/telemetry", HTTP_GET, []() {
        StaticJsonDocument<512> doc;
        doc["rpm"] = gInstantaneousRpm;
        doc["avgRpm"] = gAverageRpm;
        doc["maxRpm"] = gMaxRpm;
        doc["gear"] = gCurrentGear;
        doc["mode"] = (int)gMode;
        doc["isShifting"] = (gShiftState != SHIFT_IDLE);
        doc["clutchDisengaged"] = gClutchDisengaged;
        doc["stepperPosition"] = gStepperCurrentPosition;
        doc["engineTorqueCut"] = gTorqueCut;
        doc["emergencyActive"] = gEmergencyActive;
        doc["totalShifts"] = gTotalShifts;
        doc["speedKmh"] = estimateVehicleSpeedKmh(gCurrentGear, gAverageRpm);
        String s; serializeJson(doc, s);
        httpServer.send(200, "application/json", s);
    });

    httpServer.on("/api/control/gear", HTTP_POST, []() {
        if (!httpServer.hasArg("plain")) { httpServer.send(400); return; }
        StaticJsonDocument<256> doc;
        deserializeJson(doc, httpServer.arg("plain"));
        bool ok = false;
        if (doc.containsKey("action")) {
            const char* act = doc["action"];
            if (strcmp(act, "UP") == 0 && gCurrentGear < 4) ok = requestGearShift(gCurrentGear + 1);
            else if (strcmp(act, "DOWN") == 0 && gCurrentGear > 0) ok = requestGearShift(gCurrentGear - 1);
        } else if (doc.containsKey("gear")) {
            ok = requestGearShift(doc["gear"]);
        }
        httpServer.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"error\":\"rejected\"}");
    });

    httpServer.on("/api/control/mode", HTTP_POST, []() {
        if (!httpServer.hasArg("plain")) { httpServer.send(400); return; }
        StaticJsonDocument<128> doc;
        deserializeJson(doc, httpServer.arg("plain"));
        gMode = (doc["mode"] == 1) ? MODE_AUTOMATIC : MODE_MANUAL;
        httpServer.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    httpServer.on("/api/control/estop", HTTP_POST, []() {
        if (!httpServer.hasArg("plain")) { httpServer.send(400); return; }
        StaticJsonDocument<128> doc;
        deserializeJson(doc, httpServer.arg("plain"));
        bool act = doc["active"] | false;
        if (act) triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "Web UI E-Stop");
        else clearEmergencyStop();
        httpServer.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    httpServer.begin();
    wsServer.begin();
    Serial.println("[HTTP] Embedded WebServer running on port 80 & WebSockets on port 81.");
}

// =================================================================================================
// 11. SETUP & INITIALIZATION
// =================================================================================================
void initDefaultConfig() {
    config.pulsesPerRevolution = 1;
    config.glitchFilterMicros = 200;
    config.sensorTimeoutMs = 2500;
    config.maxPlausibleRpm = 12000;
    config.rpmCalibrationFactor = 1.0f;

    config.clutchServoChannel = 0;
    config.clutchEngagedAngle = 15;     // 15 deg = Engaged
    config.clutchDisengagedAngle = 95;  // 95 deg = Disengaged (Pulled)
    config.clutchTravelTimeMs = 250;

    config.maxSpeedStepsPerSec = 800;
    config.gearPositions[0] = 0;     // Neutral
    config.gearPositions[1] = 500;   // 1st Gear
    config.gearPositions[2] = 1000;  // 2nd Gear
    config.gearPositions[3] = 1500;  // 3rd Gear
    config.gearPositions[4] = 2000;  // 4th Gear
    config.stepperTimeoutMs = 2500;
    config.useStepDirMode = false;

    config.engineRelayActiveHigh = false;
    config.engineCutDurationMs = 90;

    config.clutchPullDelayMs = 250;
    config.engineCutPreDelayMs = 40;
    config.engineRestoreDelayMs = 60;
    config.clutchReleaseDelayMs = 280;
    config.minShiftIntervalMs = 3000;

    config.autoThresholds[1] = { 3200, 1300, 15.0f };
    config.autoThresholds[2] = { 3400, 1500, 30.0f };
    config.autoThresholds[3] = { 3600, 1700, 50.0f };
    config.autoThresholds[4] = { 4000, 1900, 75.0f };
    config.hysteresisRpm = 250;
    config.minEngineRpm = 900;
    config.redlineRpm = 6500;

    strncpy(config.wifiSsid, AMT_DEFAULT_WIFI_SSID, sizeof(config.wifiSsid) - 1);
    strncpy(config.wifiPassword, AMT_DEFAULT_WIFI_PASS, sizeof(config.wifiPassword) - 1);
    strncpy(config.apSsid, AMT_DEFAULT_AP_SSID, sizeof(config.apSsid) - 1);
    strncpy(config.apPassword, AMT_DEFAULT_AP_PASS, sizeof(config.apPassword) - 1);
    strncpy(config.mqttBroker, AMT_DEFAULT_MQTT_HOST, sizeof(config.mqttBroker) - 1);
    config.mqttPort = AMT_DEFAULT_MQTT_PORT;
    strncpy(config.mqttUser, "amt_user", sizeof(config.mqttUser) - 1);
    strncpy(config.mqttPass, "amt_password", sizeof(config.mqttPass) - 1);
    strncpy(config.mqttClientId, "ESP32_AMT_01", sizeof(config.mqttClientId) - 1);

    config.simulationMode = false;
    config.estopNormallyClosed = true;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================================");
    Serial.println("  ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM");
    Serial.printf("  Firmware: %s | Microcontroller: ESP32S 38-Pin\n", AMT_FIRMWARE_VER);
    Serial.println("========================================================");

    initDefaultConfig();

    // 1. Safety Hardware E-Stop pin
    pinMode(PIN_ESTOP_HW, INPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH);

    // 2. Engine cut relay
    pinMode(PIN_ENGINE_RELAY, OUTPUT);
    setEngineRelay(false);

    // 3. Stepper outputs
    pinMode(PIN_STEPPER_IN1, OUTPUT);
    pinMode(PIN_STEPPER_IN2, OUTPUT);
    pinMode(PIN_STEPPER_IN3, OUTPUT);
    pinMode(PIN_STEPPER_IN4, OUTPUT);
    enableStepperCoils(false);

    // 4. PCA9685 I2C Servo Driver
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
    pca9685.begin();
    pca9685.setOscillatorFrequency(27000000);
    pca9685.setPWMFreq(50);
    engageClutch(); // Start with clutch engaged (15 deg)

    // 5. RPM Sensor Interrupt on GPIO 34
    pinMode(PIN_RPM_SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_RPM_SENSOR), rpmSensorIsr, FALLING);

    // 6. Wi-Fi Station & SoftAP Fallback
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(config.wifiSsid, config.wifiPassword);

    uint32_t startAttempt = millis();
    Serial.printf("[NETWORK] Connecting to Wi-Fi SSID: %s ", config.wifiSsid);
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 6000) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[NETWORK] Connected! IP: %s, RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        gIsApMode = false;
    } else {
        Serial.println("\n[NETWORK] Connection timed out. Starting fallback SoftAP mode...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(config.apSsid, config.apPassword);
        gIsApMode = true;
        Serial.printf("[NETWORK] AP Active: %s (IP: %s)\n", config.apSsid, WiFi.softAPIP().toString().c_str());
    }

    if (MDNS.begin("amt")) {
        Serial.println("[NETWORK] mDNS active: http://amt.local");
    }

    // 7. Embedded WebServer & WebSockets
    registerRestRoutes();

    // 8. MQTT Gateway
    mqttClient.setServer(config.mqttBroker, config.mqttPort);
    mqttClient.setBufferSize(1024);
    mqttClient.setCallback(onMqttMessageReceived);

    Serial.println("[MAIN] AMT Controller initialization complete. Running real-time loop.\n");
}

// =================================================================================================
// 12. HARD REAL-TIME SUPERVISORY LOOP
// =================================================================================================
void loop() {
    uint32_t nowMs = millis();
    uint32_t nowMicros = micros();

    // -------------------------------------------------------------------------
    // A. Hardware E-Stop Supervision
    // -------------------------------------------------------------------------
    bool pinState = digitalRead(PIN_ESTOP_HW);
    bool isPressed = config.estopNormallyClosed ? (pinState == HIGH) : (pinState == LOW);

    if (isPressed && !gEmergencyActive) {
        if (nowMs - gEstopDebounceMs > 50) {
            triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "Hardware E-Stop button pressed!");
        }
    } else {
        gEstopDebounceMs = nowMs;
    }

    // Status LED indication
    if (gEmergencyActive) {
        digitalWrite(PIN_STATUS_LED, (nowMs / 100) % 2 == 0 ? HIGH : LOW); // Fast strobe
    } else {
        digitalWrite(PIN_STATUS_LED, HIGH);
    }

    // -------------------------------------------------------------------------
    // B. RPM Sensor Calculation & Noise Filter
    // -------------------------------------------------------------------------
    uint32_t lastPulse, intervalMicros;
    noInterrupts();
    lastPulse = s_lastPulseMicros;
    intervalMicros = s_pulseIntervalMicros;
    interrupts();

    if ((nowMs - gLastPulseTimeMs) > config.sensorTimeoutMs) {
        gInstantaneousRpm = 0.0f;
        gAverageRpm = gAverageRpm * 0.85f;
        if (gAverageRpm < 50.0f) gAverageRpm = 0.0f;
        gSensorConnected = false;
    } else {
        gSensorConnected = true;
    }

    if (intervalMicros > 0 && (micros() - lastPulse) < (config.sensorTimeoutMs * 1000UL)) {
        float rawRpm = (60000000.0f / (float)(intervalMicros * config.pulsesPerRevolution)) * config.rpmCalibrationFactor;
        if (rawRpm <= config.maxPlausibleRpm) {
            gInstantaneousRpm = rawRpm;
            gLastPulseTimeMs = nowMs;
            if (gInstantaneousRpm > gMaxRpm) gMaxRpm = gInstantaneousRpm;

            gRpmHistory[gRpmHistoryIdx] = gInstantaneousRpm;
            gRpmHistoryIdx = (gRpmHistoryIdx + 1) % AVG_SAMPLES;
            float sum = 0.0f;
            for (uint8_t i = 0; i < AVG_SAMPLES; i++) sum += gRpmHistory[i];
            gAverageRpm = sum / (float)AVG_SAMPLES;
        }
    }

    // -------------------------------------------------------------------------
    // C. Non-Blocking Stepper Motor Stepping
    // -------------------------------------------------------------------------
    if (gStepperMoving) {
        if ((nowMs - gStepperMoveStartMs) > config.stepperTimeoutMs) {
            gStepperStalled = true;
            gStepperMoving = false;
            enableStepperCoils(false);
            Serial.println("[STEPPER] ERROR: Movement timeout (possible mechanical stall)!");
        } else if ((nowMicros - gLastStepMicros) >= gStepIntervalMicros) {
            gLastStepMicros = nowMicros;
            if (gStepperCurrentPosition < gStepperTargetPosition) {
                gStepperPhase = (gStepperPhase + 1) % 8;
                applyStepperPhase(gStepperPhase);
                gStepperCurrentPosition++;
            } else if (gStepperCurrentPosition > gStepperTargetPosition) {
                gStepperPhase = (gStepperPhase == 0) ? 7 : (gStepperPhase - 1);
                applyStepperPhase(gStepperPhase);
                gStepperCurrentPosition--;
            }

            if (gStepperCurrentPosition == gStepperTargetPosition) {
                gStepperMoving = false;
                gLastMoveCompletedMs = nowMs;
            }
        }
    } else {
        // Coil power-saving timeout: de-energize after 400ms idle to prevent L298N thermal overload
        if (gCoilsEnergized && (nowMs - gLastMoveCompletedMs > 400)) {
            enableStepperCoils(false);
        }
    }

    // -------------------------------------------------------------------------
    // D. Non-Blocking Engine Torque Cut Relay Timer
    // -------------------------------------------------------------------------
    if (gTorqueCut && !gEmergencyActive) {
        if ((nowMs - gCutStartTimeMs) >= gCutDurationMs) {
            restoreTorque();
        }
    }

    // -------------------------------------------------------------------------
    // E. Transmission State Machine
    // -------------------------------------------------------------------------
    updateTransmissionStateMachine();

    // -------------------------------------------------------------------------
    // F. WebServer & WebSockets Telemetry Broadcast (10 Hz)
    // -------------------------------------------------------------------------
    httpServer.handleClient();
    wsServer.loop();

    if (nowMs - gLastWsBroadcastMs >= 100) {
        gLastWsBroadcastMs = nowMs;
        StaticJsonDocument<512> doc;
        doc["rpm"] = gInstantaneousRpm;
        doc["avg_rpm"] = gAverageRpm;
        doc["max_rpm"] = gMaxRpm;
        doc["gear"] = gCurrentGear;
        doc["target_gear"] = gTargetGear;
        doc["shifting"] = (gShiftState != SHIFT_IDLE);
        doc["mode"] = (int)gMode;
        doc["clutch_disengaged"] = gClutchDisengaged;
        doc["stepper_pos"] = gStepperCurrentPosition;
        doc["engine_cut"] = gTorqueCut;
        doc["estop"] = gEmergencyActive;
        doc["total_shifts"] = gTotalShifts;
        doc["speed_kmh"] = estimateVehicleSpeedKmh(gCurrentGear, gAverageRpm);
        String s; serializeJson(doc, s);
        wsServer.broadcastTXT(s);
    }

    // -------------------------------------------------------------------------
    // G. MQTT Client Loop & Periodic Telemetry
    // -------------------------------------------------------------------------
    if (!gIsApMode && WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            if (nowMs - gLastMqttReconnectAttemptMs > 3000) {
                gLastMqttReconnectAttemptMs = nowMs;
                connectMqttBroker();
            }
        } else {
            mqttClient.loop();

            // Periodic telemetry publish (10 Hz)
            if (nowMs - gLastTelemetryPublishMs >= 100) {
                gLastTelemetryPublishMs = nowMs;
                publishTelemetryPacket();
            }

            // Periodic heartbeat (1 Hz)
            if (nowMs - gLastHeartbeatMs >= 1000) {
                gLastHeartbeatMs = nowMs;
                StaticJsonDocument<128> doc;
                doc["beat"] = nowMs;
                doc["uptime_sec"] = nowMs / 1000;
                doc["free_heap"] = ESP.getFreeHeap();
                doc["wifi_rssi"] = WiFi.RSSI();
                char buf[128];
                serializeJson(doc, buf);
                mqttClient.publish("amt/heartbeat", buf);
            }
        }
    }

    yield();
}

