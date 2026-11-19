#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "amt_config.h"

class ServoController {
public:
    ServoController();
    bool begin(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddress = 0x40);
    void update(); // Handles non-blocking angle interpolation

    // Clutch Control
    void disengageClutch(); // Pull clutch
    void engageClutch();    // Release clutch
    bool isClutchDisengaged() const;
    bool isClutchMoving() const;

    // Direct Channel Control
    void setChannelAngle(uint8_t channel, float targetAngle, uint32_t transitTimeMs = 0);
    float getChannelAngle(uint8_t channel) const;
    float getTargetAngle(uint8_t channel) const;

    // Calibration
    void calibrateClutchAngles(uint16_t engagedAngle, uint16_t disengagedAngle);

    // Diagnostics
    bool isDriverReady() const;

private:
    Adafruit_PWMServoDriver _pwm;
    bool _driverReady;

    struct ServoChannelState {
        float currentAngle;
        float targetAngle;
        float startAngle;
        uint32_t startTimeMs;
        uint32_t durationMs;
        bool inMotion;
        uint16_t minPulseUs;
        uint16_t maxPulseUs;
    } _channels[2];

    uint16_t angleToPulseUs(uint8_t ch, float angle) const;
    uint16_t pulseUsToTicks(uint16_t pulseUs) const;
    void applyRawPulse(uint8_t channel, uint16_t pulseUs);
};

extern ServoController gServoController;

#endif // SERVO_CONTROLLER_H

