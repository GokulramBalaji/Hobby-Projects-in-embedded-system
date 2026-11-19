#include "engine_controller.h"

EngineController gEngineController;

EngineController::EngineController()
    : _pin(PIN_ENGINE_RELAY),
      _torqueCut(false),
      _engineRunning(true),
      _emergencyKilled(false),
      _cutStartTimeMs(0),
      _cutDurationMs(0) {}

void EngineController::begin(uint8_t relayPin) {
    _pin = relayPin;
    pinMode(_pin, OUTPUT);
    // Initial state: Engine enabled (no cut)
    setRelayState(false);
    _torqueCut = false;
    _engineRunning = true;
    _emergencyKilled = false;
    Serial.println("[ENGINE] Engine torque management relay initialized.");
}

void EngineController::update() {
    if (_emergencyKilled) {
        setRelayState(true);
        return;
    }

    if (_torqueCut) {
        if ((millis() - _cutStartTimeMs) >= _cutDurationMs) {
            restoreTorque();
        }
    }
}

void EngineController::cutTorque(uint32_t durationMs) {
    if (_emergencyKilled) return;

    _torqueCut = true;
    _cutStartTimeMs = millis();
    _cutDurationMs = durationMs;
    setRelayState(true);
    Serial.printf("[ENGINE] Torque cut active for %u ms.\n", durationMs);
}

void EngineController::restoreTorque() {
    if (_emergencyKilled) return;

    _torqueCut = false;
    setRelayState(false);
    Serial.println("[ENGINE] Torque restored.");
}

void EngineController::emergencyKill() {
    _emergencyKilled = true;
    _torqueCut = true;
    _engineRunning = false;
    setRelayState(true);
    Serial.println("[ENGINE] EMERGENCY KILL ACTIVATED. Engine ignition/fuel cut latched.");
}

void EngineController::restartEngine() {
    _emergencyKilled = false;
    _torqueCut = false;
    _engineRunning = true;
    setRelayState(false);
    Serial.println("[ENGINE] Engine cut cleared. Engine restored.");
}

bool EngineController::isTorqueCut() const {
    return _torqueCut || _emergencyKilled;
}

bool EngineController::isEngineRunning() const {
    return _engineRunning && !_emergencyKilled;
}

bool EngineController::isEmergencyKilled() const {
    return _emergencyKilled;
}

void EngineController::setRelayState(bool cutIgnition) {
    // If active high: HIGH cuts ignition. If active low: LOW cuts ignition.
    bool pinLevel;
    if (gConfig.engineRelayActiveHigh) {
        pinLevel = cutIgnition ? HIGH : LOW;
    } else {
        pinLevel = cutIgnition ? LOW : HIGH;
    }
    digitalWrite(_pin, pinLevel);
}

