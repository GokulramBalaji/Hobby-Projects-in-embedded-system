#ifndef AMT_WEB_SERVER_H
#define AMT_WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "amt_config.h"

class AmtWebServer {
public:
    AmtWebServer();
    void begin();
    void update();
    void broadcastTelemetry(const String& jsonPayload);

private:
    WebServer _server;
    WebSocketsServer _wsServer;
    uint32_t _lastBroadcastMs;

    void registerRoutes();
    void handleRoot();
    void handleApiTelemetry();
    void handleApiControlGear();
    void handleApiControlMode();
    void handleApiControlClutch();
    void handleApiControlStepper();
    void handleApiControlEstop();
    void handleApiGetConfig();
    void handleApiSetConfig();
    void handleApiReset();
    void handleNotFound();

    static void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
};

extern AmtWebServer gAmtWebServer;

#endif // AMT_WEB_SERVER_H

