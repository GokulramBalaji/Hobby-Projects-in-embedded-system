#include "transmission.h"
#include "rpm_sensor.h"
#include "servo_controller.h"
#include "stepper_controller.h"
#include "engine_controller.h"
#include "safety_manager.h"
#include "mqtt_client.h"

Transmission gTransmission;

Transmission::Transmission()
    : _currentGear(0),
      _targetGear(0),
      _mode(MODE_MANUAL),
      _shiftState(SHIFT_IDLE),
      _stateStartTimeMs(0),
      _lastShiftCompleteTimeMs(0),
      _shiftStartTimeMs(0),
      _lastShiftDurationMs(0),
      _totalShifts(0),
      _successfulShifts(0),
      _failedShifts(0) {}

void Transmission::begin() {
    _currentGear = 0; // Starts in Neutral
    _targetGear = 0;
    _shiftState = SHIFT_IDLE;
    _mode = MODE_MANUAL;
    _lastShiftCompleteTimeMs = millis();
    Serial.println("[TRANSMISSION] Initialized in Neutral (Gear 0), Manual Mode.");
}

void Transmission::update() {
    uint32_t nowMs = millis();

    // Check emergency stop
    if (gSafetyManager.isEmergencyActive()) {
        if (_shiftState != SHIFT_EMERGENCY_ABORT && _shiftState != SHIFT_IDLE) {
            abortShift(FAULT_ESTOP_ACTIVE);
        }
        return;
    }

    // Evaluate automatic gear shifts if in auto mode and idle
    if (_mode == MODE_AUTOMATIC && _shiftState == SHIFT_IDLE) {
        evaluateAutoShift(gRpmSensor.getAverageRpm());
    }

    // Shift Sequencing State Machine
    switch (_shiftState) {
        case SHIFT_IDLE:
            break;

        case SHIFT_VALIDATING: {
            if (!validateShiftRequest(_targetGear)) {
                abortShift(FAULT_SHIFT_OVERLAP_BLOCKED);
                break;
            }
            transitionTo(SHIFT_CLUTCH_DISENGAGING);
            break;
        }

        case SHIFT_CLUTCH_DISENGAGING: {
            if (nowMs - _stateStartTimeMs == 0) {
                // Command clutch pull
                gServoController.disengageClutch();
            }
            if ((nowMs - _stateStartTimeMs) >= gConfig.clutchPullDelayMs) {
                transitionTo(SHIFT_ENGINE_CUTTING);
            }
            break;
        }

        case SHIFT_ENGINE_CUTTING: {
            if (nowMs - _stateStartTimeMs == 0) {
                // Cut engine torque
                gEngineController.cutTorque(gConfig.engineCutDurationMs);
            }
            if ((nowMs - _stateStartTimeMs) >= gConfig.engineCutPreDelayMs) {
                transitionTo(SHIFT_STEPPER_MOVING);
            }
            break;
        }

        case SHIFT_STEPPER_MOVING: {
            gStepperController.moveToGear(_targetGear);
            transitionTo(SHIFT_VERIFYING_STEPPER);
            break;
        }

        case SHIFT_VERIFYING_STEPPER: {
            // Check for stepper timeout or stall
            if (gStepperController.isStalledOrTimedOut()) {
                Serial.println("[TRANSMISSION] Stepper actuator stalled during shift!");
                abortShift(FAULT_STEPPER_TIMEOUT);
                break;
            }

            // Check if stepper reached position
            if (!gStepperController.isMoving()) {
                transitionTo(SHIFT_ENGINE_RESTORING);
            }
            break;
        }

        case SHIFT_ENGINE_RESTORING: {
            if (nowMs - _stateStartTimeMs == 0) {
                gEngineController.restoreTorque();
            }
            if ((nowMs - _stateStartTimeMs) >= gConfig.engineRestoreDelayMs) {
                transitionTo(SHIFT_CLUTCH_ENGAGING);
            }
            break;
        }

        case SHIFT_CLUTCH_ENGAGING: {
            if (nowMs - _stateStartTimeMs == 0) {
                gServoController.engageClutch();
            }
            if ((nowMs - _stateStartTimeMs) >= gConfig.clutchReleaseDelayMs) {
                transitionTo(SHIFT_VERIFYING_GEAR);
            }
            break;
        }

        case SHIFT_VERIFYING_GEAR: {
            int32_t expectedPos = gConfig.gearPositions[_targetGear];
            int32_t actualPos = gStepperController.getCurrentPosition();

            if (abs(actualPos - expectedPos) <= 20) {
                uint8_t oldGear = _currentGear;
                _currentGear = _targetGear;
                _lastShiftDurationMs = millis() - _shiftStartTimeMs;
                _successfulShifts++;
                _totalShifts++;
                _lastShiftCompleteTimeMs = millis();

                Serial.printf("[TRANSMISSION] Shift successful: Gear %d -> %d in %u ms.\n",
                              oldGear, _currentGear, _lastShiftDurationMs);

                gAmtMqttClient.publishShiftEvent(oldGear, _currentGear, true, _lastShiftDurationMs);
                transitionTo(SHIFT_COMPLETED);
            } else {
                Serial.println("[TRANSMISSION] Verification failed: gear position mismatch!");
                abortShift(FAULT_STEPPER_TIMEOUT);
            }
            break;
        }

        case SHIFT_COMPLETED: {
            transitionTo(SHIFT_IDLE);
            break;
        }

        case SHIFT_FAILED:
        case SHIFT_EMERGENCY_ABORT: {
            // Restore safety states
            gEngineController.restoreTorque();
            gServoController.engageClutch();
            _failedShifts++;
            _totalShifts++;
            _lastShiftCompleteTimeMs = millis();
            transitionTo(SHIFT_IDLE);
            break;
        }
    }
}

bool Transmission::requestGear(uint8_t targetGear, bool isAutoRequest) {
    if (targetGear > 4) return false;
    if (_shiftState != SHIFT_IDLE) {
        Serial.println("[TRANSMISSION] Request rejected: Shift already in progress.");
        return false;
    }
    if (targetGear == _currentGear) {
        return false;
    }

    // Check minimum shift interval cooldown
    if ((millis() - _lastShiftCompleteTimeMs) < gConfig.minShiftIntervalMs) {
        Serial.println("[TRANSMISSION] Request rejected: Minimum shift interval active.");
        return false;
    }

    _targetGear = targetGear;
    _shiftStartTimeMs = millis();
    transitionTo(SHIFT_VALIDATING);
    return true;
}

bool Transmission::requestUpshift() {
    if (_currentGear < 4) {
        return requestGear(_currentGear + 1);
    }
    return false;
}

bool Transmission::requestDownshift() {
    if (_currentGear > 0) {
        return requestGear(_currentGear - 1);
    }
    return false;
}

bool Transmission::requestNeutral() {
    return requestGear(0);
}

void Transmission::abortShift(FaultCode reason) {
    Serial.printf("[TRANSMISSION] Shift aborted! Reason code: %d\n", reason);
    uint8_t oldGear = _currentGear;
    _shiftState = SHIFT_FAILED;
    gAmtMqttClient.publishShiftEvent(oldGear, _targetGear, false, millis() - _shiftStartTimeMs);
    gSafetyManager.triggerEmergencyStop(reason, "Shift sequence aborted due to actuator or condition fault.");
}

void Transmission::setMode(TransmissionMode mode) {
    _mode = mode;
    Serial.printf("[TRANSMISSION] Mode set to: %s\n", (_mode == MODE_AUTOMATIC) ? "AUTOMATIC" : "MANUAL");
}

TransmissionMode Transmission::getMode() const {
    return _mode;
}

uint8_t Transmission::getCurrentGear() const {
    return _currentGear;
}

uint8_t Transmission::getTargetGear() const {
    return _targetGear;
}

ShiftState Transmission::getShiftState() const {
    return _shiftState;
}

bool Transmission::isShifting() const {
    return _shiftState != SHIFT_IDLE && _shiftState != SHIFT_COMPLETED;
}

uint32_t Transmission::getLastShiftDurationMs() const {
    return _lastShiftDurationMs;
}

uint32_t Transmission::getTotalShiftCount() const {
    return _totalShifts;
}

uint32_t Transmission::getSuccessfulShiftCount() const {
    return _successfulShifts;
}

uint32_t Transmission::getFailedShiftCount() const {
    return _failedShifts;
}

float Transmission::getSimulatedSpeedKmh() const {
    return estimateVehicleSpeed(_currentGear, gRpmSensor.getAverageRpm());
}

void Transmission::evaluateAutoShift(float currentRpm) {
    if (_shiftState != SHIFT_IDLE) return;
    if ((millis() - _lastShiftCompleteTimeMs) < gConfig.minShiftIntervalMs) return;

    // In neutral, do not auto-shift unless requested
    if (_currentGear == 0) return;

    // Check upshift threshold
    if (_currentGear < 4) {
        uint32_t upThreshold = gConfig.autoThresholds[_currentGear].upshiftRpm;
        if (currentRpm >= upThreshold) {
            Serial.printf("[AUTO-SHIFT] RPM %0.1f >= %u. Triggering Upshift to Gear %d.\n",
                          currentRpm, upThreshold, _currentGear + 1);
            requestGear(_currentGear + 1, true);
            return;
        }
    }

    // Check downshift threshold with hysteresis
    if (_currentGear > 1) {
        uint32_t downThreshold = gConfig.autoThresholds[_currentGear].downshiftRpm;
        if (currentRpm <= (downThreshold - gConfig.hysteresisRpm)) {
            Serial.printf("[AUTO-SHIFT] RPM %0.1f <= %u. Triggering Downshift to Gear %d.\n",
                          currentRpm, downThreshold, _currentGear - 1);
            requestGear(_currentGear - 1, true);
            return;
        }
    }
}

bool Transmission::validateShiftRequest(uint8_t targetGear) {
    if (targetGear > 4) return false;
    if (gSafetyManager.isEmergencyActive()) return false;

    // Redline protection on downshift:
    // If downshifting, ensure calculated RPM in lower gear won't exceed redline
    if (targetGear < _currentGear && targetGear > 0) {
        float gearRatios[5] = { 0.0f, 3.4f, 2.1f, 1.4f, 1.0f };
        float projectedRpm = gRpmSensor.getAverageRpm() * (gearRatios[targetGear] / gearRatios[_currentGear]);
        if (projectedRpm > gConfig.redlineRpm) {
            Serial.printf("[TRANSMISSION] Downshift rejected: Projected RPM %.0f exceeds redline %u!\n",
                          projectedRpm, gConfig.redlineRpm);
            return false;
        }
    }
    return true;
}

void Transmission::transitionTo(ShiftState newState) {
    _shiftState = newState;
    _stateStartTimeMs = millis();
}

float Transmission::estimateVehicleSpeed(uint8_t gear, float rpm) const {
    if (gear == 0 || rpm < 200.0f) return 0.0f;
    // Approximated gear overall speed multipliers (km/h per 1000 RPM)
    const float gearSpeedFactors[5] = { 0.0f, 5.0f, 10.0f, 16.0f, 22.0f };
    return (rpm / 1000.0f) * gearSpeedFactors[gear];
}

