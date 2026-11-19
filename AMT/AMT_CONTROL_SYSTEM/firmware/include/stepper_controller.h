#ifndef STEPPER_CONTROLLER_H
#define STEPPER_CONTROLLER_H

#include <Arduino.h>
#include "amt_config.h"

class StepperController {
public:
    StepperController();
    void begin();
    void update(); // Non-blocking step pulse generator

    // Positioning
    void moveToGear(uint8_t gear);
    void moveToPosition(int32_t targetStep);
    void jogSteps(int32_t relativeSteps);
    void stopImmediately();

    // Homing & Calibration
    void startHoming();
    void setZeroPosition();
    void calibrateGearPosition(uint8_t gear, int32_t stepPosition);

    // Status Getters
    int32_t getCurrentPosition() const;
    int32_t getTargetPosition() const;
    bool isMoving() const;
    bool isHomed() const;
    bool isStalledOrTimedOut() const;
    uint8_t getCurrentGearEstimate() const;

    // Power Management
    void enableOutputs(bool enable);

private:
    int32_t _currentPosition;
    int32_t _targetPosition;
    bool _isMoving;
    bool _isHomed;
    bool _stalled;
    uint8_t _stepPhase; // 0 to 7 (half step) or 0 to 3 (full step)

    uint32_t _lastStepMicros;
    uint32_t _stepIntervalMicros;
    uint32_t _moveStartTimeMs;

    // Power saving / coil de-energize timer
    uint32_t _lastMoveCompletedMs;
    bool _coilsEnergized;

    void executeStep(int8_t direction);
    void applyL298NPhase(uint8_t phase);
    void applyStepDirPulse();
};

extern StepperController gStepperController;

#endif // STEPPER_CONTROLLER_H

