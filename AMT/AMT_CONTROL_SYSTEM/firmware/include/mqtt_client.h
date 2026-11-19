#ifndef AMT_MQTT_CLIENT_H
#define AMT_MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "amt_config.h"

class AmtMqttClient {
public:
    AmtMqttClient();
    void begin();
    void update(); // Keeps MQTT loop alive, handles publish intervals

    bool isConnected();
    void publishTelemetry();
    void publishShiftEvent(uint8_t fromGear, uint8_t toGear, bool success, uint32_t durationMs);
    void publishFault(FaultCode code, const char* message);
    void publishEmergency(bool active, const char* reason);
    void publishHeartbeat();

private:
    WiFiClient _wifiClient;
    PubSubClient _mqtt;
    uint32_t _lastReconnectAttemptMs;
    uint32_t _lastTelemetryPublishMs;
    uint32_t _lastHeartbeatMs;

    void connectMqtt();
    void subscribeTopics();
    static void onMqttMessage(char* topic, byte* payload, unsigned int length);
    void processCommand(const JsonDocument& doc);
    void processConfigUpdate(const JsonDocument& doc);
};

extern AmtMqttClient gAmtMqttClient;

#endif // AMT_MQTT_CLIENT_H

