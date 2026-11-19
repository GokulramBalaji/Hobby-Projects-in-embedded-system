#include "web_server.h"
#include "rpm_sensor.h"
#include "servo_controller.h"
#include "stepper_controller.h"
#include "engine_controller.h"
#include "transmission.h"
#include "safety_manager.h"
#include "network_manager.h"

AmtWebServer gAmtWebServer;

// Embedded HTML Dashboard stored in PROGMEM
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 AMT Standalone Controller</title>
    <style>
        :root {
            --bg: #0b0f19; --card: #151c2e; --accent: #2563eb; --accent-glow: #3b82f6;
            --text: #f8fafc; --text-muted: #94a3b8; --danger: #ef4444; --success: #22c55e;
            --warning: #f59e0b; --border: #1e293b;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
        .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--border); padding-bottom: 12px; margin-bottom: 20px; }
        .title { font-size: 1.25rem; font-weight: 700; letter-spacing: 0.5px; }
        .status-pill { padding: 4px 10px; border-radius: 999px; font-size: 0.75rem; font-weight: 600; background: #1e293b; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 16px; margin-bottom: 20px; }
        .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 18px; display: flex; flex-direction: column; }
        .card-header { font-size: 0.85rem; text-transform: uppercase; color: var(--text-muted); font-weight: 600; margin-bottom: 8px; }
        .big-val { font-size: 2.8rem; font-weight: 800; color: #fff; line-height: 1.1; }
        .gear-badge { font-size: 4rem; font-weight: 900; color: var(--accent-glow); text-align: center; margin: 10px 0; }
        .unit { font-size: 0.9rem; color: var(--text-muted); font-weight: normal; margin-left: 4px; }
        .controls { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin-top: 10px; }
        button { background: var(--border); color: #fff; border: 1px solid #334155; padding: 12px; border-radius: 8px; font-weight: 600; font-size: 0.95rem; cursor: pointer; transition: 0.15s; }
        button:hover { background: #334155; }
        button.primary { background: var(--accent); border-color: var(--accent-glow); }
        button.primary:hover { background: #1d4ed8; }
        button.danger { background: var(--danger); border-color: #f87171; grid-column: span 2; padding: 16px; font-size: 1.1rem; }
        button.danger:hover { background: #dc2626; }
        .status-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #1e293b; font-size: 0.85rem; }
        .active-dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; background: var(--success); margin-right: 6px; }
    </style>
</head>
<body>
    <div class="header">
        <div class="title">⚡ ESP32 AMT ON-BOARD CONTROLLER</div>
        <div id="connStatus" class="status-pill"><span class="active-dot"></span>CONNECTING...</div>
    </div>

    <div class="grid">
        <!-- RPM Card -->
        <div class="card">
            <div class="card-header">Engine Rotational Speed</div>
            <div class="big-val"><span id="rpmVal">0</span><span class="unit">RPM</span></div>
            <div class="status-row" style="margin-top:auto;"><span>Avg RPM:</span><b id="avgRpm">0</b></div>
            <div class="status-row"><span>Sensor Status:</span><b id="sensorStat">Active</b></div>
        </div>

        <!-- Gear Card -->
        <div class="card" style="text-align: center;">
            <div class="card-header">Active Gear Position</div>
            <div id="gearVal" class="gear-badge">N</div>
            <div id="shiftState" style="font-size:0.8rem; color:var(--text-muted); font-weight:600;">STANDBY</div>
        </div>

        <!-- Transmission Controls -->
        <div class="card">
            <div class="card-header">Manual / Auto Override</div>
            <div class="controls">
                <button class="primary" onclick="sendShift('UP')">▲ UPSHIFT</button>
                <button class="primary" onclick="sendShift('DOWN')">▼ DOWNSHIFT</button>
                <button onclick="sendGear(0)">NEUTRAL (N)</button>
                <button onclick="toggleMode()" id="modeBtn">MODE: MANUAL</button>
            </div>
        </div>

        <!-- Actuator Diagnostics -->
        <div class="card">
            <div class="card-header">Actuators & Interlocks</div>
            <div class="status-row"><span>Clutch Servo:</span><b id="clutchStat">Engaged (15°)</b></div>
            <div class="status-row"><span>Stepper Motor:</span><b id="stepperStat">0 Steps</b></div>
            <div class="status-row"><span>Engine Torque Cut:</span><b id="engineStat">Normal (Active)</b></div>
            <div class="status-row"><span>Total Shifts:</span><b id="shiftsCount">0</b></div>
        </div>
    </div>

    <button class="danger" onclick="toggleEstop()" id="estopBtn">🚨 EMERGENCY STOP</button>

    <script>
        let ws;
        let isAuto = false;
        let isEstop = false;

        function connectWs() {
            const host = window.location.hostname || "192.168.4.1";
            ws = new WebSocket("ws://" + host + ":81/");
            ws.onopen = () => {
                document.getElementById("connStatus").innerHTML = '<span class="active-dot" style="background:#22c55e"></span>ONLINE (WS)';
            };
            ws.onclose = () => {
                document.getElementById("connStatus").innerHTML = '<span class="active-dot" style="background:#ef4444"></span>OFFLINE';
                setTimeout(connectWs, 2000);
            };
            ws.onmessage = (evt) => {
                try {
                    const data = JSON.parse(evt.data);
                    document.getElementById("rpmVal").innerText = Math.round(data.rpm);
                    document.getElementById("avgRpm").innerText = Math.round(data.avg_rpm);
                    
                    const gears = ["N", "1", "2", "3", "4"];
                    document.getElementById("gearVal").innerText = gears[data.gear] || "?";
                    document.getElementById("shiftState").innerText = data.shifting ? "SHIFTING IN PROGRESS..." : "STANDBY";
                    
                    document.getElementById("clutchStat").innerText = data.clutch_disengaged ? "Disengaged" : "Engaged";
                    document.getElementById("stepperStat").innerText = data.stepper_pos + " Steps";
                    document.getElementById("engineStat").innerText = data.engine_cut ? "TORQUE CUT" : "Running";
                    document.getElementById("shiftsCount").innerText = data.total_shifts;

                    isAuto = (data.mode === 1);
                    document.getElementById("modeBtn").innerText = isAuto ? "MODE: AUTO" : "MODE: MANUAL";

                    isEstop = data.estop;
                    const estopBtn = document.getElementById("estopBtn");
                    if (isEstop) {
                        estopBtn.innerText = "⚠️ CLEAR EMERGENCY STOP";
                        estopBtn.style.background = "#eab308";
                    } else {
                        estopBtn.innerText = "🚨 EMERGENCY STOP";
                        estopBtn.style.background = "#ef4444";
                    }
                } catch(e) {}
            };
        }

        function sendShift(dir) {
            fetch("/api/control/gear", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({ action: dir })
            });
        }

        function sendGear(g) {
            fetch("/api/control/gear", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({ gear: g })
            });
        }

        function toggleMode() {
            fetch("/api/control/mode", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({ mode: isAuto ? 0 : 1 })
            });
        }

        function toggleEstop() {
            fetch("/api/control/estop", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({ active: !isEstop })
            });
        }

        window.onload = connectWs;
    </script>
</body>
</html>
)rawliteral";

AmtWebServer::AmtWebServer()
    : _server(80),
      _wsServer(81),
      _lastBroadcastMs(0) {}

void AmtWebServer::begin() {
    registerRoutes();
    _server.begin();

    _wsServer.begin();
    _wsServer.onEvent(onWsEvent);

    Serial.println("[HTTP] Embedded Web Server started on port 80.");
    Serial.println("[WS] WebSocket Server started on port 81.");
}

void AmtWebServer::update() {
    _server.handleClient();
    _wsServer.loop();

    uint32_t nowMs = millis();
    if (nowMs - _lastBroadcastMs >= 100) { // 10Hz live telemetry broadcast
        _lastBroadcastMs = nowMs;

        StaticJsonDocument<512> doc;
        doc["rpm"] = gRpmSensor.getInstantaneousRpm();
        doc["avg_rpm"] = gRpmSensor.getAverageRpm();
        doc["max_rpm"] = gRpmSensor.getMaxRpm();
        doc["gear"] = gTransmission.getCurrentGear();
        doc["target_gear"] = gTransmission.getTargetGear();
        doc["shifting"] = gTransmission.isShifting();
        doc["mode"] = (int)gTransmission.getMode();
        doc["clutch_disengaged"] = gServoController.isClutchDisengaged();
        doc["stepper_pos"] = gStepperController.getCurrentPosition();
        doc["engine_cut"] = gEngineController.isTorqueCut();
        doc["estop"] = gSafetyManager.isEmergencyActive();
        doc["total_shifts"] = gTransmission.getTotalShiftCount();
        doc["speed_kmh"] = gTransmission.getSimulatedSpeedKmh();

        String payload;
        serializeJson(doc, payload);
        broadcastTelemetry(payload);
    }
}

void AmtWebServer::broadcastTelemetry(const String& jsonPayload) {
    _wsServer.broadcastTXT(jsonPayload);
}

void AmtWebServer::onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.printf("[WS] Client #%u connected.\n", num);
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("[WS] Client #%u disconnected.\n", num);
    }
}

void AmtWebServer::registerRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/telemetry", HTTP_GET, [this]() { handleApiTelemetry(); });
    _server.on("/api/control/gear", HTTP_POST, [this]() { handleApiControlGear(); });
    _server.on("/api/control/mode", HTTP_POST, [this]() { handleApiControlMode(); });
    _server.on("/api/control/clutch", HTTP_POST, [this]() { handleApiControlClutch(); });
    _server.on("/api/control/stepper", HTTP_POST, [this]() { handleApiControlStepper(); });
    _server.on("/api/control/estop", HTTP_POST, [this]() { handleApiControlEstop(); });
    _server.on("/api/config", HTTP_GET, [this]() { handleApiGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this]() { handleApiSetConfig(); });
    _server.on("/api/reset", HTTP_POST, [this]() { handleApiReset(); });
    _server.onNotFound([this]() { handleNotFound(); });
}

void AmtWebServer::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

void AmtWebServer::handleApiTelemetry() {
    StaticJsonDocument<512> doc;
    doc["rpm"] = gRpmSensor.getInstantaneousRpm();
    doc["avgRpm"] = gRpmSensor.getAverageRpm();
    doc["maxRpm"] = gRpmSensor.getMaxRpm();
    doc["gear"] = gTransmission.getCurrentGear();
    doc["mode"] = (int)gTransmission.getMode();
    doc["isShifting"] = gTransmission.isShifting();
    doc["clutchDisengaged"] = gServoController.isClutchDisengaged();
    doc["stepperPosition"] = gStepperController.getCurrentPosition();
    doc["engineTorqueCut"] = gEngineController.isTorqueCut();
    doc["emergencyActive"] = gSafetyManager.isEmergencyActive();
    doc["totalShifts"] = gTransmission.getTotalShiftCount();
    doc["successfulShifts"] = gTransmission.getSuccessfulShiftCount();
    doc["failedShifts"] = gTransmission.getFailedShiftCount();
    doc["speedKmh"] = gTransmission.getSimulatedSpeedKmh();
    doc["uptimeSec"] = millis() / 1000;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void AmtWebServer::handleApiControlGear() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"JSON parse error\"}");
        return;
    }

    if (doc.containsKey("action")) {
        const char* act = doc["action"];
        if (strcmp(act, "UP") == 0) {
            bool ok = gTransmission.requestUpshift();
            _server.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"error\":\"Rejected\"}");
            return;
        } else if (strcmp(act, "DOWN") == 0) {
            bool ok = gTransmission.requestDownshift();
            _server.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"error\":\"Rejected\"}");
            return;
        }
    }

    if (doc.containsKey("gear")) {
        uint8_t g = doc["gear"];
        bool ok = gTransmission.requestGear(g);
        _server.send(200, "application/json", ok ? "{\"status\":\"ok\"}" : "{\"error\":\"Rejected\"}");
        return;
    }

    _server.send(400, "application/json", "{\"error\":\"Invalid gear command\"}");
}

void AmtWebServer::handleApiControlMode() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, _server.arg("plain"));
    int modeVal = doc["mode"] | 0;
    gTransmission.setMode((TransmissionMode)modeVal);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AmtWebServer::handleApiControlClutch() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, _server.arg("plain"));
    if (doc.containsKey("disengaged")) {
        bool dis = doc["disengaged"];
        if (dis) gServoController.disengageClutch();
        else gServoController.engageClutch();
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AmtWebServer::handleApiControlStepper() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<256> doc;
    deserializeJson(doc, _server.arg("plain"));
    if (doc.containsKey("jog")) {
        int32_t j = doc["jog"];
        gStepperController.jogSteps(j);
    } else if (doc.containsKey("home")) {
        gStepperController.startHoming();
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AmtWebServer::handleApiControlEstop() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, _server.arg("plain"));
    bool active = doc["active"] | false;
    if (active) {
        gSafetyManager.triggerEmergencyStop(FAULT_ESTOP_ACTIVE, "Software E-Stop triggered via Web UI");
    } else {
        gSafetyManager.clearEmergencyStop();
    }
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AmtWebServer::handleApiGetConfig() {
    StaticJsonDocument<1024> doc;
    doc["ppr"] = gConfig.pulsesPerRevolution;
    doc["clutchEngagedAngle"] = gConfig.clutchEngagedAngle;
    doc["clutchDisengagedAngle"] = gConfig.clutchDisengagedAngle;
    doc["engineCutDurationMs"] = gConfig.engineCutDurationMs;
    doc["g1UpRpm"] = gConfig.autoThresholds[1].upshiftRpm;
    doc["g2UpRpm"] = gConfig.autoThresholds[2].upshiftRpm;
    doc["g3UpRpm"] = gConfig.autoThresholds[3].upshiftRpm;
    doc["wifiSsid"] = gConfig.wifiSsid;
    doc["mqttBroker"] = gConfig.mqttBroker;
    doc["mqttPort"] = gConfig.mqttPort;

    String resp;
    serializeJson(doc, resp);
    _server.send(200, "application/json", resp);
}

void AmtWebServer::handleApiSetConfig() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, _server.arg("plain"));

    if (doc.containsKey("ppr")) gConfig.pulsesPerRevolution = doc["ppr"];
    if (doc.containsKey("clutchEngagedAngle")) gConfig.clutchEngagedAngle = doc["clutchEngagedAngle"];
    if (doc.containsKey("clutchDisengagedAngle")) gConfig.clutchDisengagedAngle = doc["clutchDisengagedAngle"];
    if (doc.containsKey("engineCutDurationMs")) gConfig.engineCutDurationMs = doc["engineCutDurationMs"];
    if (doc.containsKey("g1UpRpm")) gConfig.autoThresholds[1].upshiftRpm = doc["g1UpRpm"];
    if (doc.containsKey("g2UpRpm")) gConfig.autoThresholds[2].upshiftRpm = doc["g2UpRpm"];
    if (doc.containsKey("g3UpRpm")) gConfig.autoThresholds[3].upshiftRpm = doc["g3UpRpm"];

    configSaveToStorage();
    _server.send(200, "application/json", "{\"status\":\"saved\"}");
}

void AmtWebServer::handleApiReset() {
    _server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(500);
    ESP.restart();
}

void AmtWebServer::handleNotFound() {
    _server.send(404, "text/plain", "404 Not Found");
}

