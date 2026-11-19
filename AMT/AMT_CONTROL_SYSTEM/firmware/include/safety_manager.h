#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include "amt_config.h"

class SafetyManager {
public:
    SafetyManager();
    void begin(uint8_t estopPin);
    void update(); // Polls hardware E-Stop and communication timeouts

    // Emergency Stop
    void triggerEmergencyStop(FaultCode reason, const char* message = nullptr);
    bool clearEmergencyStop(); // Requires conditions to be safe
    bool isEmergencyActive() const;
    FaultCode getLastFaultCode() const;
    const char* getLastFaultMessage() const;

    // Heartbeat & Watchdog
    void feedWatchdog();
    void recordPcHeartbeat();
    bool isPcConnected() const;

private:
    uint8_t _estopPin;
    bool _emergencyActive;
    FaultCode _lastFaultCode;
    char _faultMessage[64];
    uint32_t _lastPcHeartbeatMs;
    uint32_t _estopDebounceMs;
    bool _lastPinState;

    void executeFailsafeShutdown();
};

extern SafetyManager gSafetyManager;

#endif // SAFETY_MANAGER_H

