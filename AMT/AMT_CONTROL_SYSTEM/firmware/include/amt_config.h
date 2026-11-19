#ifndef AMT_CONFIG_H
#define AMT_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32 38-PIN LAYOUT)
// ============================================================================
// Sensor & Safety Inputs (GPIO 34-39 are input-only pins, no internal pullups)
#define PIN_RPM_SENSOR         34   // V183 IR Sensor (via voltage divider / 3.3V)
#define PIN_ESTOP_HW           35   // Hardware Emergency Stop (NC button to GND)

// I2C Bus for PCA9685 16-Channel Servo Driver
#define PIN_I2C_SDA            21   // PCA9685 SDA (Logic 3.3V)
#define PIN_I2C_SCL            22   // PCA9685 SCL (Logic 3.3V)

// Actuator Outputs: Stepper Motor (L298N 4-Wire Coil Interface & STEP/DIR fallback)
#define PIN_STEPPER_IN1        25   // L298N Coil A+ (or STEP in Step/Dir mode)
#define PIN_STEPPER_IN2        26   // L298N Coil A- (or DIR in Step/Dir mode)
#define PIN_STEPPER_IN3        27   // L298N Coil B+ (or ENABLE in Step/Dir mode)
#define PIN_STEPPER_IN4        14   // L298N Coil B-

// Engine Interruption Interface
#define PIN_ENGINE_RELAY       19   // Optocoupled Relay for ignition/fuel torque cut

// Status & Diagnostics
#define PIN_STATUS_LED         2    // On-board LED indicator

// ============================================================================
// DEFAULT CONSTANTS
// ============================================================================
#define AMT_FIRMWARE_VERSION   "2.4.0"
#define AMT_DEVICE_NAME        "ESP32-AMT-CONTROLLER"
#define DEFAULT_WIFI_SSID      "AMT_WIFI_NETWORK"
#define DEFAULT_WIFI_PASS      "amt_secure_password"
#define DEFAULT_AP_SSID        "AMT-CONTROLLER-AP"
#define DEFAULT_AP_PASS        "amtadminpassword"
#define DEFAULT_MQTT_BROKER    "192.168.1.100"
#define DEFAULT_MQTT_PORT      1883
#define DEFAULT_MQTT_USER      "amt_user"
#define DEFAULT_MQTT_PASS      "amt_password"

// Transmission Modes
enum TransmissionMode {
    MODE_MANUAL = 0,
    MODE_AUTOMATIC = 1
};

// Shifting State Machine States
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

// Fault Codes
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

// ============================================================================
// CONFIGURATION DATA STRUCTURE
// ============================================================================
struct AMTConfig {
    // RPM Sensor
    uint16_t pulsesPerRevolution;
    uint32_t glitchFilterMicros;
    uint32_t sensorTimeoutMs;
    uint32_t maxPlausibleRpm;
    float    rpmCalibrationFactor;

    // Clutch Servo (PCA9685 Channel 0)
    uint8_t  clutchServoChannel;
    uint16_t clutchEngagedAngle;     // Typically ~15-20 deg (resting/engaged)
    uint16_t clutchDisengagedAngle;  // Typically ~95-100 deg (pulled/disengaged)
    uint32_t clutchTravelTimeMs;

    // Stepper Motor (NEMA 17 via L298N)
    uint16_t stepsPerRevolution;
    uint16_t maxSpeedStepsPerSec;
    int32_t  gearPositions[5];       // 0=Neutral, 1=Gear1, 2=Gear2, 3=Gear3, 4=Gear4
    uint32_t stepperTimeoutMs;
    bool     useStepDirMode;

    // Engine Cut Relay
    bool     engineRelayActiveHigh;
    uint32_t engineCutDurationMs;

    // Shifting Timing Delays (ms)
    uint32_t clutchPullDelayMs;
    uint32_t engineCutPreDelayMs;
    uint32_t gearShiftTimeoutMs;
    uint32_t engineRestoreDelayMs;
    uint32_t clutchReleaseDelayMs;
    uint32_t minShiftIntervalMs;

    // Automatic Shifting RPM Thresholds
    struct GearThreshold {
        uint32_t upshiftRpm;
        uint32_t downshiftRpm;
        float    nominalSpeedKmh;
    } autoThresholds[5];
    uint32_t hysteresisRpm;
    uint32_t minEngineRpm;
    uint32_t redlineRpm;

    // Network & MQTT
    char     wifiSsid[33];
    char     wifiPassword[65];
    char     apSsid[33];
    char     apPassword[65];
    char     mqttBroker[65];
    uint16_t mqttPort;
    char     mqttUser[33];
    char     mqttPass[33];
    char     mqttClientId[33];

    // Simulation & Safety
    bool     simulationMode;
    bool     estopNormallyClosed;
};

// Global Configuration Instance
extern AMTConfig gConfig;

// Configuration Management Functions
void configInitDefaults();
bool configLoadFromStorage();
bool configSaveToStorage();
void configResetToFactory();

#endif // AMT_CONFIG_H

