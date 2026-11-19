#include "stepper_controller.h"

StepperController gStepperController;

// Half-stepping table for 4-wire bipolar L298N (IN1, IN2, IN3, IN4)
// Provides 400 half-steps/rev on a 1.8-degree NEMA 17 for high precision & minimal vibration
static const uint8_t L298N_HALF_STEPS[8][4] = {
    {1, 0, 0, 0},
    {1, 0, 1, 0},
    {0, 0, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

StepperController::StepperController()
    : _currentPosition(0),
      _targetPosition(0),
      _isMoving(false),
      _isHomed(false),
      _stalled(false),
      _stepPhase(0),
      _lastStepMicros(0),
      _stepIntervalMicros(1250), // ~800 steps/sec
      _moveStartTimeMs(0),
      _lastMoveCompletedMs(0),
      _coilsEnergized(false) {}

void StepperController::begin() {
    pinMode(PIN_STEPPER_IN1, OUTPUT);
    pinMode(PIN_STEPPER_IN2, OUTPUT);
    pinMode(PIN_STEPPER_IN3, OUTPUT);
    pinMode(PIN_STEPPER_IN4, OUTPUT);

    // Initial state: de-energize coils to prevent static thermal overload
    enableOutputs(false);

    _currentPosition = gConfig.gearPositions[0]; // Assume initial Neutral
    _targetPosition = _currentPosition;
    _isHomed = true;
    _stepIntervalMicros = (uint32_t)(1000000UL / gConfig.maxSpeedStepsPerSec);

    Serial.println("[STEPPER] Initialized L298N 4-phase bipolar controller.");
}

void StepperController::update() {
    uint32_t nowMicros = micros();
    uint32_t nowMs = millis();

    if (_isMoving) {
        // Check timeout
        if ((nowMs - _moveStartTimeMs) > gConfig.stepperTimeoutMs) {
            Serial.println("[STEPPER] ERROR: Step movement timed out! Possible mechanical stall.");
            _stalled = true;
            _isMoving = false;
            enableOutputs(false);
            return;
        }

        if ((nowMicros - _lastStepMicros) >= _stepIntervalMicros) {
            _lastStepMicros = nowMicros;

            if (_currentPosition < _targetPosition) {
                executeStep(1); // Forward
                _currentPosition++;
            } else if (_currentPosition > _targetPosition) {
                executeStep(-1); // Reverse
                _currentPosition--;
            }

            if (_currentPosition == _targetPosition) {
                _isMoving = false;
                _lastMoveCompletedMs = nowMs;
                Serial.printf("[STEPPER] Target step %ld reached successfully.\n", (long)_currentPosition);
            }
        }
    } else {
        // Coil power-saving timeout: de-energize after 400ms idle to keep L298N cold
        if (_coilsEnergized && (nowMs - _lastMoveCompletedMs > 400)) {
            enableOutputs(false);
        }
    }
}

void StepperController::moveToGear(uint8_t gear) {
    if (gear > 4) return;
    moveToPosition(gConfig.gearPositions[gear]);
}

void StepperController::moveToPosition(int32_t targetStep) {
    if (_targetPosition == targetStep && _currentPosition == targetStep) {
        return;
    }
    _targetPosition = targetStep;
    _moveStartTimeMs = millis();
    _stalled = false;
    _isMoving = true;
    enableOutputs(true);
}

void StepperController::jogSteps(int32_t relativeSteps) {
    moveToPosition(_currentPosition + relativeSteps);
}

void StepperController::stopImmediately() {
    _targetPosition = _currentPosition;
    _isMoving = false;
    enableOutputs(false);
}

void StepperController::startHoming() {
    Serial.println("[STEPPER] Starting homing sequence towards neutral (0)...");
    _targetPosition = 0;
    _isMoving = true;
    _moveStartTimeMs = millis();
    enableOutputs(true);
}

void StepperController::setZeroPosition() {
    _currentPosition = 0;
    _targetPosition = 0;
    _isHomed = true;
    Serial.println("[STEPPER] Zero position calibrated as Neutral (0 steps).");
}

void StepperController::calibrateGearPosition(uint8_t gear, int32_t stepPosition) {
    if (gear <= 4) {
        gConfig.gearPositions[gear] = stepPosition;
        Serial.printf("[STEPPER] Gear %d calibrated to %ld steps.\n", gear, (long)stepPosition);
    }
}

int32_t StepperController::getCurrentPosition() const {
    return _currentPosition;
}

int32_t StepperController::getTargetPosition() const {
    return _targetPosition;
}

bool StepperController::isMoving() const {
    return _isMoving;
}

bool StepperController::isHomed() const {
    return _isHomed;
}

bool StepperController::isStalledOrTimedOut() const {
    return _stalled;
}

uint8_t StepperController::getCurrentGearEstimate() const {
    int32_t minDiff = 999999;
    uint8_t closestGear = 0;
    for (uint8_t g = 0; g <= 4; g++) {
        int32_t diff = abs(_currentPosition - gConfig.gearPositions[g]);
        if (diff < minDiff) {
            minDiff = diff;
            closestGear = g;
        }
    }
    return (minDiff < 50) ? closestGear : 0;
}

void StepperController::enableOutputs(bool enable) {
    _coilsEnergized = enable;
    if (!enable) {
        digitalWrite(PIN_STEPPER_IN1, LOW);
        digitalWrite(PIN_STEPPER_IN2, LOW);
        digitalWrite(PIN_STEPPER_IN3, LOW);
        digitalWrite(PIN_STEPPER_IN4, LOW);
    }
}

void StepperController::executeStep(int8_t direction) {
    if (gConfig.useStepDirMode) {
        applyStepDirPulse();
    } else {
        if (direction > 0) {
            _stepPhase = (_stepPhase + 1) % 8;
        } else {
            _stepPhase = (_stepPhase == 0) ? 7 : (_stepPhase - 1);
        }
        applyL298NPhase(_stepPhase);
    }
}

void StepperController::applyL298NPhase(uint8_t phase) {
    if (phase >= 8) {
        enableOutputs(false);
        return;
    }
    digitalWrite(PIN_STEPPER_IN1, L298N_HALF_STEPS[phase][0] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN2, L298N_HALF_STEPS[phase][1] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN3, L298N_HALF_STEPS[phase][2] ? HIGH : LOW);
    digitalWrite(PIN_STEPPER_IN4, L298N_HALF_STEPS[phase][3] ? HIGH : LOW);
}

void StepperController::applyStepDirPulse() {
    // Mode for A4988/TMC2209 Step/Dir drivers
    digitalWrite(PIN_STEPPER_IN2, (_targetPosition > _currentPosition) ? HIGH : LOW); // DIR
    digitalWrite(PIN_STEPPER_IN1, HIGH); // STEP pulse
    delayMicroseconds(5);
    digitalWrite(PIN_STEPPER_IN1, LOW);
}

