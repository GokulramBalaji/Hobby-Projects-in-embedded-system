#ifndef RPM_SENSOR_H
#define RPM_SENSOR_H

#include <Arduino.h>
#include "amt_config.h"

class RpmSensor {
public:
    RpmSensor();
    void begin(uint8_t pin);
    void update(); // Called regularly in main loop

    // Telemetry Getters
    float getInstantaneousRpm() const;
    float getAverageRpm() const;
    float getMaxRpm() const;
    uint32_t getTotalPulses() const;
    bool isConnected() const;
    bool isImplausible() const;
    void resetMaxRpm();
    void resetTotalPulses();

    // Configuration
    void setPulsesPerRevolution(uint16_t ppr);
    void setGlitchFilterMicros(uint32_t microsVal);
    void setCalibrationFactor(float factor);

    // Simulation injector (for bench-testing without real sensor)
    void injectSimulatedPulse(float simulatedRpm);

    // ISR Handler (must be static or friend)
    static void IRAM_ATTR isrHandler();

private:
    uint8_t _pin;
    float _instantaneousRpm;
    float _averageRpm;
    float _maxRpm;
    bool _connected;
    bool _implausible;
    uint32_t _lastPulseTimeMs;

    // Moving average buffer
    static const uint8_t AVG_SAMPLES = 8;
    float _rpmHistory[AVG_SAMPLES];
    uint8_t _historyIndex;
    uint8_t _sampleCount;

    // Static variables for ISR
    static volatile uint32_t s_pulseCount;
    static volatile uint32_t s_lastPulseMicros;
    static volatile uint32_t s_pulseIntervalMicros;
    static volatile uint32_t s_validPulseCount;
};

extern RpmSensor gRpmSensor;

#endif // RPM_SENSOR_H

