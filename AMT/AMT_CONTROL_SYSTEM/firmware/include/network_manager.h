#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "amt_config.h"

enum NetworkStatus {
    NET_DISCONNECTED = 0,
    NET_CONNECTING = 1,
    NET_STA_CONNECTED = 2,
    NET_AP_MODE = 3
};

class NetworkManager {
public:
    NetworkManager();
    void begin();
    void update(); // Handles reconnection

    NetworkStatus getStatus() const;
    String getIpAddress() const;
    int8_t getRssi() const;
    bool isConnected() const;
    bool isApMode() const;

    void updateCredentials(const char* ssid, const char* password);

private:
    NetworkStatus _status;
    uint32_t _lastReconnectAttemptMs;
    uint8_t _connectionFailures;
    void startStation();
    void startAccessPoint();
};

extern NetworkManager gNetworkManager;

#endif // NETWORK_MANAGER_H

