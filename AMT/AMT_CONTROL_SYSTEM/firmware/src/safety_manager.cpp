#include "safety_manager.h"
#include "engine_controller.h"
#include "servo_controller.h"
#include "stepper_controller.h"
#include "mqtt_client.h"

SafetyManager gSafetyManager;

SafetyManager::SafetyManager()
    : _estopPin(PIN_ESTOP_HW),
      _emergencyActive(false),
      _lastFaultCode(FAULT_NONE),
      _lastPcHeartbeatMs(0),
      _estopDebounceMs(0),
      _lastPinState(HIGH) {
    _faultMessage[0] = '\0';
}

void SafetyManager::begin(uint8_t estopPin) {
    _estopPin = estopPin;
    pinMode(_estopPin, INPUT); // Note: external pullup resistor installed on hardware pin
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH);

    _emergencyActive = false;
    _lastFaultCode = FAULT_NONE;
    _lastPcHeartbeatMs = millis();
    _lastPinState = digitalRead(_estopPin);

    Serial.println("[SAFETY] Safety supervisor initialized. Hardware E-Stop monitored.");
}

void SafetyManager::update() {
    uint32_t nowMs = millis();

    // Check Hardware E-Stop switch
    bool pinState = digitalRead(_estopPin);
    // Normally Closed (NC) switch: button pressed or wire broken = pin reads HIGH (with pullup)
    bool isPressed = gConfig.estopNormallyClosed ? (pinState == HIGH) : (pinState == LOW);

    if (isPressed && !_emergencyActive) {
        if (nowMs - _estopDebounceMs > 50) { // 50ms debounce
            triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "Hardware Emergency Stop button pressed!");
        }
    } else {
        _estopDebounceMs = nowMs;
    }

    // Emergency Status LED indication
    if (_emergencyActive) {
        // Fast blink (10Hz) when emergency stop active
        digitalWrite(PIN_STATUS_LED, (nowMs / 100) % 2 == 0 ? HIGH : LOW);
    } else {
        // Solid ON or slow pulse in normal operation
        digitalWrite(PIN_STATUS_LED, HIGH);
    }
}

void SafetyManager::triggerEmergencyStop(FaultCode reason, const char* message) {
    _emergencyActive = true;
    _lastFaultCode = reason;
    if (message) {
        strncpy(_faultMessage, message, sizeof(_faultMessage) - 1);
    } else {
        snprintf(_faultMessage, sizeof(_faultMessage), "Emergency Stop triggered (Code: %d)", reason);
    }

    Serial.printf("[SAFETY] *** CRITICAL EMERGENCY STOP TRIGGERED *** Reason: %s\n", _faultMessage);
    executeFailsafeShutdown();
    gAmtMqttClient.publishEmergency(true, _faultMessage);
    gAmtMqttClient.publishFault(_lastFaultCode, _faultMessage);
}

bool SafetyManager::clearEmergencyStop() {
    // Check if hardware button is still pressed
    bool pinState = digitalRead(_estopPin);
    bool isPressed = gConfig.estopNormallyClosed ? (pinState == HIGH) : (pinState == LOW);

    if (isPressed) {
        Serial.println("[SAFETY] Cannot clear E-Stop: Hardware switch is still physically active!");
        return false;
    }

    _emergencyActive = false;
    _lastFaultCode = FAULT_NONE;
    _faultMessage[0] = '\0';

    // Safely restore systems without moving motors
    gEngineController.restartEngine();
    gServoController.engageClutch();
    digitalWrite(PIN_STATUS_LED, HIGH);

    Serial.println("[SAFETY] Emergency Stop cleared. System returned to standby.");
    gAmtMqttClient.publishEmergency(false, "Emergency cleared by operator.");
    return true;
}

bool SafetyManager::isEmergencyActive() const {
    return _emergencyActive;
}

FaultCode SafetyManager::getLastFaultCode() const {
    return _lastFaultCode;
}

const char* SafetyManager::getLastFaultMessage() const {
    return _faultMessage;
}

void SafetyManager::feedWatchdog() {
    // Reset hardware/task watchdog
}

void SafetyManager::recordPcHeartbeat() {
    _lastPcHeartbeatMs = millis();
}

bool SafetyManager::isPcConnected() const {
    return (millis() - _lastPcHeartbeatMs) < 6000;
}

void SafetyManager::executeFailsafeShutdown() {
    // 1. Immediately cut engine ignition/fuel
    gEngineController.emergencyKill();

    // 2. Immediately stop stepper motor movement & de-energize coils
    gStepperController.stopImmediately();

    // 3. Move clutch to disengaged safety angle
    gServoController.disengageClutch();
}

