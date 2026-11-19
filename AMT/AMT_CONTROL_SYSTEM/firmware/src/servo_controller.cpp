#include "servo_controller.h"

ServoController gServoController;

ServoController::ServoController()
    : _pwm(0x40),
      _driverReady(false) {
    for (uint8_t i = 0; i < 2; i++) {
        _channels[i].currentAngle = 15.0f;
        _channels[i].targetAngle = 15.0f;
        _channels[i].startAngle = 15.0f;
        _channels[i].startTimeMs = 0;
        _channels[i].durationMs = 0;
        _channels[i].inMotion = false;
        _channels[i].minPulseUs = 500;
        _channels[i].maxPulseUs = 2500;
    }
}

bool ServoController::begin(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddress) {
    Wire.begin(sdaPin, sclPin, 400000); // 400kHz Fast I2C

    // Probe I2C address
    Wire.beginTransmission(i2cAddress);
    if (Wire.endTransmission() != 0) {
        Serial.printf("[SERVO] Warning: PCA9685 at 0x%02X not responding on I2C!\n", i2cAddress);
        _driverReady = false;
        return false;
    }

    _pwm = Adafruit_PWMServoDriver(i2cAddress);
    _pwm.begin();
    _pwm.setOscillatorFrequency(27000000); // 27 MHz internal oscillator calibration
    _pwm.setPWMFreq(50); // 50 Hz standard analog/digital servo PWM frequency
    delay(10);

    _driverReady = true;

    // Initialize clutch servo to engaged rest position (15 degrees)
    setChannelAngle(gConfig.clutchServoChannel, gConfig.clutchEngagedAngle, 100);

    Serial.println("[SERVO] PCA9685 initialized successfully at 50Hz.");
    return true;
}

void ServoController::update() {
    uint32_t now = millis();

    for (uint8_t ch = 0; ch < 2; ch++) {
        if (_channels[ch].inMotion) {
            uint32_t elapsed = now - _channels[ch].startTimeMs;
            if (elapsed >= _channels[ch].durationMs || _channels[ch].durationMs == 0) {
                _channels[ch].currentAngle = _channels[ch].targetAngle;
                _channels[ch].inMotion = false;
            } else {
                float progress = (float)elapsed / (float)_channels[ch].durationMs;
                // Linear interpolation
                _channels[ch].currentAngle = _channels[ch].startAngle + 
                    (_channels[ch].targetAngle - _channels[ch].startAngle) * progress;
            }

            uint16_t pulseUs = angleToPulseUs(ch, _channels[ch].currentAngle);
            applyRawPulse(ch, pulseUs);
        }
    }
}

void ServoController::disengageClutch() {
    setChannelAngle(gConfig.clutchServoChannel, gConfig.clutchDisengagedAngle, gConfig.clutchTravelTimeMs);
}

void ServoController::engageClutch() {
    setChannelAngle(gConfig.clutchServoChannel, gConfig.clutchEngagedAngle, gConfig.clutchTravelTimeMs);
}

bool ServoController::isClutchDisengaged() const {
    uint8_t ch = gConfig.clutchServoChannel;
    return abs(_channels[ch].currentAngle - gConfig.clutchDisengagedAngle) < 5.0f;
}

bool ServoController::isClutchMoving() const {
    return _channels[gConfig.clutchServoChannel].inMotion;
}

void ServoController::setChannelAngle(uint8_t channel, float targetAngle, uint32_t transitTimeMs) {
    if (channel >= 2) return;

    if (targetAngle < 0.0f) targetAngle = 0.0f;
    if (targetAngle > 180.0f) targetAngle = 180.0f;

    _channels[channel].startAngle = _channels[channel].currentAngle;
    _channels[channel].targetAngle = targetAngle;
    _channels[channel].startTimeMs = millis();
    _channels[channel].durationMs = transitTimeMs;
    _channels[channel].inMotion = true;

    if (transitTimeMs == 0) {
        _channels[channel].currentAngle = targetAngle;
        _channels[channel].inMotion = false;
        applyRawPulse(channel, angleToPulseUs(channel, targetAngle));
    }
}

float ServoController::getChannelAngle(uint8_t channel) const {
    if (channel >= 2) return 0.0f;
    return _channels[channel].currentAngle;
}

float ServoController::getTargetAngle(uint8_t channel) const {
    if (channel >= 2) return 0.0f;
    return _channels[channel].targetAngle;
}

void ServoController::calibrateClutchAngles(uint16_t engagedAngle, uint16_t disengagedAngle) {
    gConfig.clutchEngagedAngle = engagedAngle;
    gConfig.clutchDisengagedAngle = disengagedAngle;
}

bool ServoController::isDriverReady() const {
    return _driverReady;
}

uint16_t ServoController::angleToPulseUs(uint8_t ch, float angle) const {
    uint16_t minUs = _channels[ch].minPulseUs;
    uint16_t maxUs = _channels[ch].maxPulseUs;
    return minUs + (uint16_t)((angle / 180.0f) * (maxUs - minUs));
}

uint16_t ServoController::pulseUsToTicks(uint16_t pulseUs) const {
    // 50Hz = 20,000 us period. 4096 ticks per period.
    return (uint16_t)(((uint32_t)pulseUs * 4096UL) / 20000UL);
}

void ServoController::applyRawPulse(uint8_t channel, uint16_t pulseUs) {
    if (!_driverReady) return;
    uint16_t ticks = pulseUsToTicks(pulseUs);
    _pwm.setPWM(channel, 0, ticks);
}

