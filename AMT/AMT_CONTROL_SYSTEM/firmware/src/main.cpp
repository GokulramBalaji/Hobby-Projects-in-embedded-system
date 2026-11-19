#include <Arduino.h>
#include "amt_config.h"
#include "rpm_sensor.h"
#include "servo_controller.h"
#include "stepper_controller.h"
#include "engine_controller.h"
#include "transmission.h"
#include "safety_manager.h"
#include "network_manager.h"
#include "web_server.h"
#include "mqtt_client.h"

static uint32_t sLastDebugPrintMs = 0;
static uint32_t sLastSimUpdateMs = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================================");
    Serial.println("  ADVANCED IoT AUTOMATIC MANUAL TRANSMISSION (AMT) SYSTEM");
    Serial.printf("  Firmware Version: %s | Hardware: ESP32S 38-Pin\n", AMT_FIRMWARE_VERSION);
    Serial.println("========================================================");

    // 1. Load persistent configuration from NVS flash
    configLoadFromStorage();

    // 2. Initialize Safety Supervisor first (Hardware E-stop monitoring)
    gSafetyManager.begin(PIN_ESTOP_HW);

    // 3. Initialize Engine Relay (ensure cut is inactive initially)
    gEngineController.begin(PIN_ENGINE_RELAY);

    // 4. Initialize PCA9685 I2C Servo Driver
    bool servoOk = gServoController.begin(PIN_I2C_SDA, PIN_I2C_SCL, 0x40);
    if (!servoOk) {
        Serial.println("[MAIN] WARNING: PCA9685 not detected. Check I2C wiring (SDA=21, SCL=22) and external servo power.");
    }

    // 5. Initialize Stepper Motor Controller (L298N 4-wire sequence)
    gStepperController.begin();

    // 6. Initialize RPM IR Sensor on GPIO 34
    gRpmSensor.begin(PIN_RPM_SENSOR);

    // 7. Initialize AMT Transmission State Machine (Neutral start)
    gTransmission.begin();

    // 8. Initialize Wi-Fi Network & Access Point fallback
    gNetworkManager.begin();

    // 9. Initialize ESP32 Embedded Web Dashboard & WebSockets
    gAmtWebServer.begin();

    // 10. Initialize MQTT Client for PC supervisory connection
    gAmtMqttClient.begin();

    Serial.println("[MAIN] AMT Controller initialization complete. Running real-time supervisory loop.\n");
}

void loop() {
    // High-priority Safety Update
    gSafetyManager.update();

    // Sensor Readings & Noise Filtering
    gRpmSensor.update();

    // Real-time Actuator Controllers
    gServoController.update();
    gStepperController.update();
    gEngineController.update();

    // AMT State Machine Execution
    gTransmission.update();

    // Network & Communication Supervisors
    gNetworkManager.update();
    gAmtWebServer.update();
    gAmtMqttClient.update();

    // Firmware Simulation Mode (Bench testing without physical motor/engine)
    if (gConfig.simulationMode) {
        uint32_t now = millis();
        if (now - sLastSimUpdateMs >= 50) {
            sLastSimUpdateMs = now;
            // Generate dynamic RPM based on gear
            static float simRpm = 1000.0f;
            static int simDir = 1;
            uint8_t g = gTransmission.getCurrentGear();
            if (g == 0) {
                simRpm = 950.0f + (sin(now / 500.0f) * 50.0f); // Idle RPM
            } else {
                simRpm += simDir * 45.0f;
                if (simRpm > 4200.0f) simDir = -1;
                if (simRpm < 1200.0f) simDir = 1;
            }
            gRpmSensor.injectSimulatedPulse(simRpm);
        }
    }

    // Periodic Serial Diagnostics (every 2 seconds)
    uint32_t nowMs = millis();
    if (nowMs - sLastDebugPrintMs >= 2000) {
        sLastDebugPrintMs = nowMs;
        Serial.printf("[STATUS] RPM: %.0f | Gear: %d | ShiftState: %d | Mode: %s | Clutch: %s | Stepper: %ld | MQTT: %s | Heap: %d B\n",
                      gRpmSensor.getAverageRpm(),
                      gTransmission.getCurrentGear(),
                      (int)gTransmission.getShiftState(),
                      (gTransmission.getMode() == MODE_AUTOMATIC) ? "AUTO" : "MANUAL",
                      gServoController.isClutchDisengaged() ? "DISENGAGED" : "ENGAGED",
                      (long)gStepperController.getCurrentPosition(),
                      gAmtMqttClient.isConnected() ? "CONNECTED" : "OFFLINE",
                      ESP.getFreeHeap());
    }

    // Minimal yield to allow ESP32 FreeRTOS background tasks (Wi-Fi, TCP/IP stack)
    yield();
}

