#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include <Arduino.h>
#include "amt_config.h"

class Transmission {
public:
    Transmission();
    void begin();
    void update(); // Main state machine tick

    // Shifting Commands
    bool requestGear(uint8_t targetGear, bool isAutoRequest = false);
    bool requestUpshift();
    bool requestDownshift();
    bool requestNeutral();
    void abortShift(FaultCode reason);

    // Modes
    void setMode(TransmissionMode mode);
    TransmissionMode getMode() const;

    // Status Getters
    uint8_t getCurrentGear() const;
    uint8_t getTargetGear() const;
    ShiftState getShiftState() const;
    bool isShifting() const;
    uint32_t getLastShiftDurationMs() const;
    uint32_t getTotalShiftCount() const;
    uint32_t getSuccessfulShiftCount() const;
    uint32_t getFailedShiftCount() const;
    float getSimulatedSpeedKmh() const;

    // Automatic Shifting Logic
    void evaluateAutoShift(float currentRpm);

private:
    uint8_t _currentGear;
    uint8_t _targetGear;
    TransmissionMode _mode;
    ShiftState _shiftState;
    uint32_t _stateStartTimeMs;
    uint32_t _lastShiftCompleteTimeMs;
    uint32_t _shiftStartTimeMs;
    uint32_t _lastShiftDurationMs;

    uint32_t _totalShifts;
    uint32_t _successfulShifts;
    uint32_t _failedShifts;

    bool validateShiftRequest(uint8_t targetGear);
    void transitionTo(ShiftState newState);
    float estimateVehicleSpeed(uint8_t gear, float rpm) const;
};

extern Transmission gTransmission;

#endif // TRANSMISSION_H

