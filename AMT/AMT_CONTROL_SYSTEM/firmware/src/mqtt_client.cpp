#include "mqtt_client.h"
#include "rpm_sensor.h"
#include "servo_controller.h"
#include "stepper_controller.h"
#include "engine_controller.h"
#include "transmission.h"
#include "safety_manager.h"
#include "network_manager.h"

AmtMqttClient gAmtMqttClient;

AmtMqttClient::AmtMqttClient()
    : _mqtt(_wifiClient),
      _lastReconnectAttemptMs(0),
      _lastTelemetryPublishMs(0),
      _lastHeartbeatMs(0) {}

void AmtMqttClient::begin() {
    _mqtt.setServer(gConfig.mqttBroker, gConfig.mqttPort);
    _mqtt.setBufferSize(1024); // Large packet buffer for full telemetry JSON
    _mqtt.setCallback(onMqttMessage);
    Serial.printf("[MQTT] Configured for broker: %s:%d\n", gConfig.mqttBroker, gConfig.mqttPort);
}

void AmtMqttClient::update() {
    if (!gNetworkManager.isConnected()) return;

    if (!_mqtt.connected()) {
        uint32_t nowMs = millis();
        if (nowMs - _lastReconnectAttemptMs > 3000) {
            _lastReconnectAttemptMs = nowMs;
            connectMqtt();
        }
    } else {
        _mqtt.loop();

        uint32_t nowMs = millis();

        // 1. Regular Telemetry publish (10Hz / 100ms)
        if (nowMs - _lastTelemetryPublishMs >= 100) {
            _lastTelemetryPublishMs = nowMs;
            publishTelemetry();
        }

        // 2. Heartbeat publish (1Hz / 1000ms)
        if (nowMs - _lastHeartbeatMs >= 1000) {
            _lastHeartbeatMs = nowMs;
            publishHeartbeat();
        }
    }
}

bool AmtMqttClient::isConnected() {
    return _mqtt.connected();
}

void AmtMqttClient::connectMqtt() {
    Serial.print("[MQTT] Connecting to broker...");
    // Will message: publishes offline status on amt/status if connection drops
    const char* willTopic = "amt/status";
    const char* willPayload = "{\"status\":\"offline\",\"device\":\"ESP32_AMT\"}";
    
    bool connected = false;
    if (strlen(gConfig.mqttUser) > 0) {
        connected = _mqtt.connect(gConfig.mqttClientId, gConfig.mqttUser, gConfig.mqttPass,
                                  willTopic, 1, true, willPayload);
    } else {
        connected = _mqtt.connect(gConfig.mqttClientId, willTopic, 1, true, willPayload);
    }

    if (connected) {
        Serial.println(" Connected!");
        subscribeTopics();

        // Announce online status
        StaticJsonDocument<256> doc;
        doc["status"] = "online";
        doc["device"] = gConfig.mqttClientId;
        doc["version"] = AMT_FIRMWARE_VERSION;
        doc["ip"] = gNetworkManager.getIpAddress();
        doc["rssi"] = gNetworkManager.getRssi();
        char buf[256];
        serializeJson(doc, buf);
        _mqtt.publish("amt/status", buf, true);
    } else {
        Serial.printf(" Failed, rc=%d. Will retry.\n", _mqtt.state());
    }
}

void AmtMqttClient::subscribeTopics() {
    _mqtt.subscribe("amt/commands");
    _mqtt.subscribe("amt/config");
    _mqtt.subscribe("amt/heartbeat");
    _mqtt.subscribe("amt/emergency");
    Serial.println("[MQTT] Subscribed to amt/commands, amt/config, amt/heartbeat, amt/emergency.");
}

void AmtMqttClient::publishTelemetry() {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();
    doc["rpm"] = (float)round(gRpmSensor.getInstantaneousRpm() * 10) / 10.0;
    doc["avg_rpm"] = (float)round(gRpmSensor.getAverageRpm() * 10) / 10.0;
    doc["max_rpm"] = (float)round(gRpmSensor.getMaxRpm() * 10) / 10.0;
    doc["gear"] = gTransmission.getCurrentGear();
    doc["target_gear"] = gTransmission.getTargetGear();
    doc["shift_state"] = (int)gTransmission.getShiftState();
    doc["mode"] = (gTransmission.getMode() == MODE_AUTOMATIC) ? "AUTO" : "MANUAL";
    doc["clutch_angle"] = gServoController.getChannelAngle(gConfig.clutchServoChannel);
    doc["clutch_disengaged"] = gServoController.isClutchDisengaged();
    doc["stepper_position"] = gStepperController.getCurrentPosition();
    doc["stepper_moving"] = gStepperController.isMoving();
    doc["engine_cut"] = gEngineController.isTorqueCut();
    doc["emergency"] = gSafetyManager.isEmergencyActive();
    doc["speed_kmh"] = (float)round(gTransmission.getSimulatedSpeedKmh() * 10) / 10.0;
    doc["total_shifts"] = gTransmission.getTotalShiftCount();

    char buffer[512];
    serializeJson(doc, buffer);
    _mqtt.publish("amt/telemetry", buffer);

    // Also publish to individual sub-topics as per requirements
    StaticJsonDocument<128> rpmDoc;
    rpmDoc["rpm"] = doc["rpm"];
    rpmDoc["avg"] = doc["avg_rpm"];
    rpmDoc["max"] = doc["max_rpm"];
    rpmDoc["pulses"] = gRpmSensor.getTotalPulses();
    char rpmBuf[128];
    serializeJson(rpmDoc, rpmBuf);
    _mqtt.publish("amt/rpm", rpmBuf);

    StaticJsonDocument<128> gearDoc;
    gearDoc["gear"] = doc["gear"];
    gearDoc["target"] = doc["target_gear"];
    gearDoc["mode"] = doc["mode"];
    gearDoc["shifting"] = gTransmission.isShifting();
    char gearBuf[128];
    serializeJson(gearDoc, gearBuf);
    _mqtt.publish("amt/gear", gearBuf);
}

void AmtMqttClient::publishShiftEvent(uint8_t fromGear, uint8_t toGear, bool success, uint32_t durationMs) {
    if (!_mqtt.connected()) return;
    StaticJsonDocument<256> doc;
    doc["event"] = "SHIFT_EVENT";
    doc["from_gear"] = fromGear;
    doc["to_gear"] = toGear;
    doc["success"] = success;
    doc["duration_ms"] = durationMs;
    doc["rpm"] = gRpmSensor.getAverageRpm();
    doc["timestamp"] = millis();

    char buf[256];
    serializeJson(doc, buf);
    _mqtt.publish("amt/events", buf);
}

void AmtMqttClient::publishFault(FaultCode code, const char* message) {
    if (!_mqtt.connected()) return;
    StaticJsonDocument<256> doc;
    doc["event"] = "FAULT";
    doc["code"] = (int)code;
    doc["message"] = message;
    doc["timestamp"] = millis();

    char buf[256];
    serializeJson(doc, buf);
    _mqtt.publish("amt/faults", buf);
}

void AmtMqttClient::publishEmergency(bool active, const char* reason) {
    if (!_mqtt.connected()) return;
    StaticJsonDocument<256> doc;
    doc["emergency_active"] = active;
    doc["reason"] = reason;
    doc["timestamp"] = millis();

    char buf[256];
    serializeJson(doc, buf, true);
    _mqtt.publish("amt/emergency", buf, true);
}

void AmtMqttClient::publishHeartbeat() {
    if (!_mqtt.connected()) return;
    StaticJsonDocument<128> doc;
    doc["beat"] = millis();
    doc["uptime_sec"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = gNetworkManager.getRssi();

    char buf[128];
    serializeJson(doc, buf);
    _mqtt.publish("amt/heartbeat", buf);
}

void AmtMqttClient::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    char payloadStr[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    if (error) {
        Serial.printf("[MQTT] JSON parse error on topic %s\n", topic);
        return;
    }

    if (strcmp(topic, "amt/commands") == 0) {
        gAmtMqttClient.processCommand(doc);
    } else if (strcmp(topic, "amt/config") == 0) {
        gAmtMqttClient.processConfigUpdate(doc);
    } else if (strcmp(topic, "amt/heartbeat") == 0) {
        gSafetyManager.recordPcHeartbeat();
    } else if (strcmp(topic, "amt/emergency") == 0) {
        bool active = doc["emergency_active"] | false;
        if (active) {
            gSafetyManager.triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "Emergency Stop commanded via MQTT");
        } else {
            gSafetyManager.clearEmergencyStop();
        }
    }
}

void AmtMqttClient::processCommand(const JsonDocument& doc) {
    const char* cmd = doc["cmd"] | "";
    Serial.printf("[MQTT] Received command: %s\n", cmd);

    bool ack = false;

    if (strcmp(cmd, "SHIFT") == 0) {
        uint8_t g = doc["gear"] | 0;
        ack = gTransmission.requestGear(g);
    } else if (strcmp(cmd, "UPSHIFT") == 0) {
        ack = gTransmission.requestUpshift();
    } else if (strcmp(cmd, "DOWNSHIFT") == 0) {
        ack = gTransmission.requestDownshift();
    } else if (strcmp(cmd, "NEUTRAL") == 0) {
        ack = gTransmission.requestNeutral();
    } else if (strcmp(cmd, "SET_MODE") == 0) {
        const char* m = doc["mode"] | "MANUAL";
        if (strcmp(m, "AUTO") == 0) {
            gTransmission.setMode(MODE_AUTOMATIC);
        } else {
            gTransmission.setMode(MODE_MANUAL);
        }
        ack = true;
    } else if (strcmp(cmd, "CLUTCH") == 0) {
        bool disengage = doc["disengage"] | false;
        if (disengage) gServoController.disengageClutch();
        else gServoController.engageClutch();
        ack = true;
    } else if (strcmp(cmd, "STEPPER_JOG") == 0) {
        int32_t steps = doc["steps"] | 0;
        gStepperController.jogSteps(steps);
        ack = true;
    } else if (strcmp(cmd, "STEPPER_HOME") == 0) {
        gStepperController.startHoming();
        ack = true;
    } else if (strcmp(cmd, "STEPPER_SET_ZERO") == 0) {
        gStepperController.setZeroPosition();
        ack = true;
    } else if (strcmp(cmd, "CALIBRATE_GEAR") == 0) {
        uint8_t gear = doc["gear"] | 0;
        int32_t pos = doc["position"] | 0;
        gStepperController.calibrateGearPosition(gear, pos);
        configSaveToStorage();
        ack = true;
    } else if (strcmp(cmd, "ESTOP") == 0) {
        bool active = doc["active"] | true;
        if (active) {
            gSafetyManager.triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "MQTT E-Stop command");
        } else {
            gSafetyManager.clearEmergencyStop();
        }
        ack = true;
    } else if (strcmp(cmd, "RESET") == 0) {
        Serial.println("[MQTT] Remote reset commanded!");
        delay(200);
        ESP.restart();
    }

    // Publish command acknowledgement
    StaticJsonDocument<256> ackDoc;
    ackDoc["event"] = "CMD_ACK";
    ackDoc["cmd"] = cmd;
    ackDoc["status"] = ack ? "ACCEPTED" : "REJECTED";
    ackDoc["timestamp"] = millis();
    char buf[256];
    serializeJson(ackDoc, buf);
    _mqtt.publish("amt/events", buf);
}

void AmtMqttClient::processConfigUpdate(const JsonDocument& doc) {
    if (doc.containsKey("ppr")) gConfig.pulsesPerRevolution = doc["ppr"];
    if (doc.containsKey("glitch_us")) gConfig.glitchFilterMicros = doc["glitch_us"];
    if (doc.containsKey("clutchEngagedAngle")) gConfig.clutchEngagedAngle = doc["clutchEngagedAngle"];
    if (doc.containsKey("clutchDisengagedAngle")) gConfig.clutchDisengagedAngle = doc["clutchDisengagedAngle"];
    if (doc.containsKey("engineCutDurationMs")) gConfig.engineCutDurationMs = doc["engineCutDurationMs"];
    if (doc.containsKey("g1UpRpm")) gConfig.autoThresholds[1].upshiftRpm = doc["g1UpRpm"];
    if (doc.containsKey("g2UpRpm")) gConfig.autoThresholds[2].upshiftRpm = doc["g2UpRpm"];
    if (doc.containsKey("g3UpRpm")) gConfig.autoThresholds[3].upshiftRpm = doc["g3UpRpm"];
    if (doc.containsKey("minShiftIntervalMs")) gConfig.minShiftIntervalMs = doc["minShiftIntervalMs"];

    configSaveToStorage();
    Serial.println("[MQTT] Updated configuration saved to flash NVS.");
}

