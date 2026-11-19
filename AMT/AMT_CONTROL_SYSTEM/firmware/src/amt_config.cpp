#include "amt_config.h"

AMTConfig gConfig;
static Preferences sPreferences;

void configInitDefaults() {
    // RPM Sensor
    gConfig.pulsesPerRevolution = 1;
    gConfig.glitchFilterMicros = 200;
    gConfig.sensorTimeoutMs = 2500;
    gConfig.maxPlausibleRpm = 12000;
    gConfig.rpmCalibrationFactor = 1.0f;

    // Clutch Servo
    gConfig.clutchServoChannel = 0;
    gConfig.clutchEngagedAngle = 15;     // 15 deg = engaged / clutch plate pressed
    gConfig.clutchDisengagedAngle = 95;  // 95 deg = disengaged / clutch pulled
    gConfig.clutchTravelTimeMs = 250;

    // Stepper Motor
    gConfig.stepsPerRevolution = 200;
    gConfig.maxSpeedStepsPerSec = 800;
    gConfig.gearPositions[0] = 0;     // Neutral
    gConfig.gearPositions[1] = 500;   // 1st Gear
    gConfig.gearPositions[2] = 1000;  // 2nd Gear
    gConfig.gearPositions[3] = 1500;  // 3rd Gear
    gConfig.gearPositions[4] = 2000;  // 4th Gear
    gConfig.stepperTimeoutMs = 2500;
    gConfig.useStepDirMode = false;   // Default: L298N 4-wire sequence

    // Engine Cut Relay
    gConfig.engineRelayActiveHigh = false; // Active LOW common for relay modules
    gConfig.engineCutDurationMs = 90;

    // Shift Delays
    gConfig.clutchPullDelayMs = 250;
    gConfig.engineCutPreDelayMs = 40;
    gConfig.gearShiftTimeoutMs = 1200;
    gConfig.engineRestoreDelayMs = 60;
    gConfig.clutchReleaseDelayMs = 280;
    gConfig.minShiftIntervalMs = 3000;

    // Auto Shifting RPM Thresholds & Speeds
    gConfig.autoThresholds[1] = { 3200, 1300, 15.0f }; // Gear 1: 15 km/h
    gConfig.autoThresholds[2] = { 3400, 1500, 30.0f }; // Gear 2: 30 km/h
    gConfig.autoThresholds[3] = { 3600, 1700, 50.0f }; // Gear 3: 50 km/h
    gConfig.autoThresholds[4] = { 4000, 1900, 75.0f }; // Gear 4: 75 km/h
    gConfig.hysteresisRpm = 250;
    gConfig.minEngineRpm = 900;
    gConfig.redlineRpm = 6500;

    // Network & MQTT
    strncpy(gConfig.wifiSsid, DEFAULT_WIFI_SSID, sizeof(gConfig.wifiSsid) - 1);
    strncpy(gConfig.wifiPassword, DEFAULT_WIFI_PASS, sizeof(gConfig.wifiPassword) - 1);
    strncpy(gConfig.apSsid, DEFAULT_AP_SSID, sizeof(gConfig.apSsid) - 1);
    strncpy(gConfig.apPassword, DEFAULT_AP_PASS, sizeof(gConfig.apPassword) - 1);
    strncpy(gConfig.mqttBroker, DEFAULT_MQTT_BROKER, sizeof(gConfig.mqttBroker) - 1);
    gConfig.mqttPort = DEFAULT_MQTT_PORT;
    strncpy(gConfig.mqttUser, DEFAULT_MQTT_USER, sizeof(gConfig.mqttUser) - 1);
    strncpy(gConfig.mqttPass, DEFAULT_MQTT_PASS, sizeof(gConfig.mqttPass) - 1);
    strncpy(gConfig.mqttClientId, "ESP32_AMT_01", sizeof(gConfig.mqttClientId) - 1);

    // Safety & Simulation
    gConfig.simulationMode = false;
    gConfig.estopNormallyClosed = true;
}

bool configLoadFromStorage() {
    configInitDefaults();
    if (!sPreferences.begin("amt_cfg", true)) {
        return false;
    }

    if (sPreferences.isKey("cfg_init")) {
        gConfig.pulsesPerRevolution = sPreferences.getUShort("ppr", gConfig.pulsesPerRevolution);
        gConfig.glitchFilterMicros = sPreferences.getUInt("glitch_us", gConfig.glitchFilterMicros);
        gConfig.clutchEngagedAngle = sPreferences.getUShort("cl_eng", gConfig.clutchEngagedAngle);
        gConfig.clutchDisengagedAngle = sPreferences.getUShort("cl_dis", gConfig.clutchDisengagedAngle);
        gConfig.clutchTravelTimeMs = sPreferences.getUInt("cl_ms", gConfig.clutchTravelTimeMs);
        
        gConfig.gearPositions[0] = sPreferences.getInt("g0_pos", gConfig.gearPositions[0]);
        gConfig.gearPositions[1] = sPreferences.getInt("g1_pos", gConfig.gearPositions[1]);
        gConfig.gearPositions[2] = sPreferences.getInt("g2_pos", gConfig.gearPositions[2]);
        gConfig.gearPositions[3] = sPreferences.getInt("g3_pos", gConfig.gearPositions[3]);
        gConfig.gearPositions[4] = sPreferences.getInt("g4_pos", gConfig.gearPositions[4]);

        gConfig.engineCutDurationMs = sPreferences.getUInt("eng_cut_ms", gConfig.engineCutDurationMs);
        gConfig.engineRelayActiveHigh = sPreferences.getBool("eng_act_hi", gConfig.engineRelayActiveHigh);

        gConfig.autoThresholds[1].upshiftRpm = sPreferences.getUInt("g1_up", gConfig.autoThresholds[1].upshiftRpm);
        gConfig.autoThresholds[1].downshiftRpm = sPreferences.getUInt("g1_down", gConfig.autoThresholds[1].downshiftRpm);
        gConfig.autoThresholds[2].upshiftRpm = sPreferences.getUInt("g2_up", gConfig.autoThresholds[2].upshiftRpm);
        gConfig.autoThresholds[2].downshiftRpm = sPreferences.getUInt("g2_down", gConfig.autoThresholds[2].downshiftRpm);
        gConfig.autoThresholds[3].upshiftRpm = sPreferences.getUInt("g3_up", gConfig.autoThresholds[3].upshiftRpm);
        gConfig.autoThresholds[3].downshiftRpm = sPreferences.getUInt("g3_down", gConfig.autoThresholds[3].downshiftRpm);
        gConfig.autoThresholds[4].downshiftRpm = sPreferences.getUInt("g4_down", gConfig.autoThresholds[4].downshiftRpm);

        sPreferences.getString("wifi_ssid", gConfig.wifiSsid, sizeof(gConfig.wifiSsid));
        sPreferences.getString("wifi_pass", gConfig.wifiPassword, sizeof(gConfig.wifiPassword));
        sPreferences.getString("mqtt_host", gConfig.mqttBroker, sizeof(gConfig.mqttBroker));
        gConfig.mqttPort = sPreferences.getUShort("mqtt_port", gConfig.mqttPort);
    }
    sPreferences.end();
    return true;
}

bool configSaveToStorage() {
    if (!sPreferences.begin("amt_cfg", false)) {
        return false;
    }
    sPreferences.putBool("cfg_init", true);
    sPreferences.putUShort("ppr", gConfig.pulsesPerRevolution);
    sPreferences.putUInt("glitch_us", gConfig.glitchFilterMicros);
    sPreferences.putUShort("cl_eng", gConfig.clutchEngagedAngle);
    sPreferences.putUShort("cl_dis", gConfig.clutchDisengagedAngle);
    sPreferences.putUInt("cl_ms", gConfig.clutchTravelTimeMs);

    sPreferences.putInt("g0_pos", gConfig.gearPositions[0]);
    sPreferences.putInt("g1_pos", gConfig.gearPositions[1]);
    sPreferences.putInt("g2_pos", gConfig.gearPositions[2]);
    sPreferences.putInt("g3_pos", gConfig.gearPositions[3]);
    sPreferences.putInt("g4_pos", gConfig.gearPositions[4]);

    sPreferences.putUInt("eng_cut_ms", gConfig.engineCutDurationMs);
    sPreferences.putBool("eng_act_hi", gConfig.engineRelayActiveHigh);

    sPreferences.putUInt("g1_up", gConfig.autoThresholds[1].upshiftRpm);
    sPreferences.putUInt("g1_down", gConfig.autoThresholds[1].downshiftRpm);
    sPreferences.putUInt("g2_up", gConfig.autoThresholds[2].upshiftRpm);
    sPreferences.putUInt("g2_down", gConfig.autoThresholds[2].downshiftRpm);
    sPreferences.putUInt("g3_up", gConfig.autoThresholds[3].upshiftRpm);
    sPreferences.putUInt("g3_down", gConfig.autoThresholds[3].downshiftRpm);
    sPreferences.putUInt("g4_down", gConfig.autoThresholds[4].downshiftRpm);

    sPreferences.putString("wifi_ssid", gConfig.wifiSsid);
    sPreferences.putString("wifi_pass", gConfig.wifiPassword);
    sPreferences.putString("mqtt_host", gConfig.mqttBroker);
    sPreferences.putUShort("mqtt_port", gConfig.mqttPort);

    sPreferences.end();
    return true;
}

void configResetToFactory() {
    if (sPreferences.begin("amt_cfg", false)) {
        sPreferences.clear();
        sPreferences.end();
    }
    configInitDefaults();
}

