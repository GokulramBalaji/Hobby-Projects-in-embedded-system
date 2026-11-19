#ifndef ENGINE_CONTROLLER_H
#define ENGINE_CONTROLLER_H

#include <Arduino.h>
#include "amt_config.h"

class EngineController {
public:
    EngineController();
    void begin(uint8_t relayPin);
    void update(); // Non-blocking timer check

    // Torque Interruption for Gear Shifting
    void cutTorque(uint32_t durationMs);
    void restoreTorque();

    // Permanent Shutdown
    void emergencyKill();
    void restartEngine();

    // Status
    bool isTorqueCut() const;
    bool isEngineRunning() const;
    bool isEmergencyKilled() const;

private:
    uint8_t _pin;
    bool _torqueCut;
    bool _engineRunning;
    bool _emergencyKilled;
    uint32_t _cutStartTimeMs;
    uint32_t _cutDurationMs;

    void setRelayState(bool cutIgnition);
};

extern EngineController gEngineController;

#endif // ENGINE_CONTROLLER_H

