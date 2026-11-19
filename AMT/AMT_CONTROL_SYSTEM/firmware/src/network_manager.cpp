#include "network_manager.h"

NetworkManager gNetworkManager;

NetworkManager::NetworkManager()
    : _status(NET_DISCONNECTED),
      _lastReconnectAttemptMs(0),
      _connectionFailures(0) {}

void NetworkManager::begin() {
    startStation();
}

void NetworkManager::startStation() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Disable Wi-Fi modem sleep for minimal MQTT/Web latency
    WiFi.begin(gConfig.wifiSsid, gConfig.wifiPassword);
    _status = NET_CONNECTING;
    _lastReconnectAttemptMs = millis();
    Serial.printf("[NETWORK] Connecting to Wi-Fi SSID: %s ...\n", gConfig.wifiSsid);
}

void NetworkManager::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(gConfig.apSsid, gConfig.apPassword);
    _status = NET_AP_MODE;
    Serial.printf("[NETWORK] Started Fallback Access Point: %s (IP: %s)\n",
                  gConfig.apSsid, WiFi.softAPIP().toString().c_str());

    if (MDNS.begin("amt")) {
        Serial.println("[NETWORK] mDNS responder started: http://amt.local");
    }
}

void NetworkManager::update() {
    uint32_t nowMs = millis();

    if (_status == NET_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _status = NET_STA_CONNECTED;
            _connectionFailures = 0;
            Serial.printf("[NETWORK] Wi-Fi Connected! IP: %s, RSSI: %d dBm\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());

            if (MDNS.begin("amt")) {
                Serial.println("[NETWORK] mDNS responder started: http://amt.local");
            }
        } else if (nowMs - _lastReconnectAttemptMs > 12000) { // 12 second station timeout
            _connectionFailures++;
            Serial.printf("[NETWORK] Wi-Fi connection timed out (attempt %d).\n", _connectionFailures);
            if (_connectionFailures >= 2) {
                Serial.println("[NETWORK] Switching to Fallback AP mode.");
                startAccessPoint();
            } else {
                _lastReconnectAttemptMs = nowMs;
                WiFi.reconnect();
            }
        }
    } else if (_status == NET_STA_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            _status = NET_DISCONNECTED;
            _lastReconnectAttemptMs = nowMs;
            Serial.println("[NETWORK] Wi-Fi connection lost!");
        }
    } else if (_status == NET_DISCONNECTED) {
        if (nowMs - _lastReconnectAttemptMs > 5000) {
            _lastReconnectAttemptMs = nowMs;
            Serial.println("[NETWORK] Attempting Wi-Fi reconnection...");
            startStation();
        }
    }
}

NetworkStatus NetworkManager::getStatus() const {
    return _status;
}

String NetworkManager::getIpAddress() const {
    if (_status == NET_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

int8_t NetworkManager::getRssi() const {
    if (_status == NET_STA_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

bool NetworkManager::isConnected() const {
    return (_status == NET_STA_CONNECTED) || (_status == NET_AP_MODE);
}

bool NetworkManager::isApMode() const {
    return _status == NET_AP_MODE;
}

void NetworkManager::updateCredentials(const char* ssid, const char* password) {
    strncpy(gConfig.wifiSsid, ssid, sizeof(gConfig.wifiSsid) - 1);
    strncpy(gConfig.wifiPassword, password, sizeof(gConfig.wifiPassword) - 1);
    configSaveToStorage();
    startStation();
}

