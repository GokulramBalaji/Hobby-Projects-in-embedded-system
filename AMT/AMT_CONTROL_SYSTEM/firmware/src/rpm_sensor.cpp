#include "rpm_sensor.h"

RpmSensor gRpmSensor;

volatile uint32_t RpmSensor::s_pulseCount = 0;
volatile uint32_t RpmSensor::s_lastPulseMicros = 0;
volatile uint32_t RpmSensor::s_pulseIntervalMicros = 0;
volatile uint32_t RpmSensor::s_validPulseCount = 0;

void IRAM_ATTR RpmSensor::isrHandler() {
    uint32_t now = micros();
    uint32_t interval = now - s_lastPulseMicros;
    
    // Noise & bounce glitch filter rejection
    if (interval >= gConfig.glitchFilterMicros) {
        s_pulseIntervalMicros = interval;
        s_lastPulseMicros = now;
        s_pulseCount++;
        s_validPulseCount++;
    }
}

RpmSensor::RpmSensor()
    : _pin(PIN_RPM_SENSOR),
      _instantaneousRpm(0.0f),
      _averageRpm(0.0f),
      _maxRpm(0.0f),
      _connected(false),
      _implausible(false),
      _lastPulseTimeMs(0),
      _historyIndex(0),
      _sampleCount(0) {
    for (uint8_t i = 0; i < AVG_SAMPLES; i++) {
        _rpmHistory[i] = 0.0f;
    }
}

void RpmSensor::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, INPUT); // Note: GPIO 34 does not have internal pullup; external pullup/divider required
    attachInterrupt(digitalPinToInterrupt(_pin), isrHandler, FALLING);
    _lastPulseTimeMs = millis();
}

void RpmSensor::update() {
    uint32_t nowMs = millis();
    uint32_t lastPulse;
    uint32_t intervalMicros;

    noInterrupts();
    lastPulse = s_lastPulseMicros;
    intervalMicros = s_pulseIntervalMicros;
    interrupts();

    // Check timeout: if no pulse received in sensorTimeoutMs
    if ((nowMs - _lastPulseTimeMs) > gConfig.sensorTimeoutMs) {
        _instantaneousRpm = 0.0f;
        _connected = false;
    } else {
        _connected = true;
    }

    if (intervalMicros > 0 && (micros() - lastPulse) < (gConfig.sensorTimeoutMs * 1000UL)) {
        float rawRpm = (60000000.0f / (float)(intervalMicros * gConfig.pulsesPerRevolution)) 
                        * gConfig.rpmCalibrationFactor;

        // Plausibility check
        if (rawRpm > gConfig.maxPlausibleRpm) {
            _implausible = true;
            // Ignore noise spike
        } else {
            _implausible = false;
            _instantaneousRpm = rawRpm;
            _lastPulseTimeMs = nowMs;

            if (_instantaneousRpm > _maxRpm) {
                _maxRpm = _instantaneousRpm;
            }

            // Moving Average Ring Buffer
            _rpmHistory[_historyIndex] = _instantaneousRpm;
            _historyIndex = (_historyIndex + 1) % AVG_SAMPLES;
            if (_sampleCount < AVG_SAMPLES) _sampleCount++;

            float sum = 0.0f;
            for (uint8_t i = 0; i < _sampleCount; i++) {
                sum += _rpmHistory[i];
            }
            _averageRpm = sum / (float)_sampleCount;
        }
    } else {
        _instantaneousRpm = 0.0f;
        _averageRpm = _averageRpm * 0.85f; // Exponential decay to 0
        if (_averageRpm < 50.0f) _averageRpm = 0.0f;
    }
}

float RpmSensor::getInstantaneousRpm() const {
    return _instantaneousRpm;
}

float RpmSensor::getAverageRpm() const {
    return _averageRpm;
}

float RpmSensor::getMaxRpm() const {
    return _maxRpm;
}

uint32_t RpmSensor::getTotalPulses() const {
    return s_validPulseCount;
}

bool RpmSensor::isConnected() const {
    return _connected;
}

bool RpmSensor::isImplausible() const {
    return _implausible;
}

void RpmSensor::resetMaxRpm() {
    _maxRpm = 0.0f;
}

void RpmSensor::resetTotalPulses() {
    noInterrupts();
    s_validPulseCount = 0;
    interrupts();
}

void RpmSensor::setPulsesPerRevolution(uint16_t ppr) {
    if (ppr > 0) gConfig.pulsesPerRevolution = ppr;
}

void RpmSensor::setGlitchFilterMicros(uint32_t microsVal) {
    gConfig.glitchFilterMicros = microsVal;
}

void RpmSensor::setCalibrationFactor(float factor) {
    if (factor > 0.1f && factor < 10.0f) gConfig.rpmCalibrationFactor = factor;
}

void RpmSensor::injectSimulatedPulse(float simulatedRpm) {
    if (simulatedRpm <= 0.0f) {
        _instantaneousRpm = 0.0f;
        _averageRpm = 0.0f;
        return;
    }
    uint32_t interval = (uint32_t)((60000000.0f / (simulatedRpm * gConfig.pulsesPerRevolution)));
    noInterrupts();
    s_pulseIntervalMicros = interval;
    s_lastPulseMicros = micros();
    s_pulseCount++;
    s_validPulseCount++;
    interrupts();
    _lastPulseTimeMs = millis();
}

