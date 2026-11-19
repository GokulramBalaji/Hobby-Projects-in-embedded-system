/*
===============================================================================
             ESP32 SINGLE 360° CONTINUOUS SERVO WEB CONTROLLER
===============================================================================

                         MULTI-ROTATION CONTROL

  TARGET RANGE
  ------------
  -36000° to +36000°

  EXAMPLES
  --------
  +360°    = 1 clockwise rotation
  +720°    = 2 clockwise rotations
  +3600°   = 10 clockwise rotations
  +7200°   = 20 clockwise rotations
  +36000°  = 100 clockwise rotations

  -360°    = 1 counter-clockwise rotation
  -720°    = 2 counter-clockwise rotations
  -3600°   = 10 counter-clockwise rotations
  -7200°   = 20 counter-clockwise rotations
  -36000°  = 100 counter-clockwise rotations

  HARDWARE
  --------
  Servo Signal -> GPIO 26
  Servo VCC    -> External 5V
  Servo GND    -> ESP32 GND

  IMPORTANT
  ---------
  This program is designed for a CONTINUOUS-ROTATION SERVO.

  A normal 180° positional servo cannot physically perform
  unlimited rotations.

  The ESP32 estimates accumulated angle using time.

  For highly accurate positioning, an encoder/Hall sensor
  should be added.

  REQUIRED LIBRARY
  ----------------
  ESP32Servo

===============================================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>


// ============================================================================
//                              WIFI SETTINGS
// ============================================================================

const char* WIFI_SSID     = "Wifi";
const char* WIFI_PASSWORD = "aaaaaaac";


// ============================================================================
//                              SERVO SETTINGS
// ============================================================================

#define SERVO_PIN 26


// ============================================================================
//                        ROTATION LIMITS
// ============================================================================

const float MIN_ROTATION = -36000.0;

const float MAX_ROTATION =  36000.0;


// ============================================================================
//                    CONTINUOUS SERVO PWM SETTINGS
// ============================================================================

/*
   These values depend on your servo.

   Typical continuous servo:

       1500 us = STOP
       >1500 us = one direction
       <1500 us = opposite direction

   These values may need calibration.
*/

const int SERVO_STOP_US = 1500;

const int SERVO_CW_MIN_US = 1510;

const int SERVO_CW_MAX_US = 2000;

const int SERVO_CCW_MIN_US = 1490;

const int SERVO_CCW_MAX_US = 1000;


// ============================================================================
//                     ROTATION CALIBRATION
// ============================================================================

/*
   VERY IMPORTANT

   This value determines how the ESP32 estimates rotation.

   Example:

       60 degrees/sec

   means the servo is expected to rotate approximately
   60 degrees every second at the selected speed.

   You MUST calibrate this for your particular servo.

   Start with something like:

       60.0

   and adjust experimentally.
*/

float degreesPerSecondAtMaxSpeed = 360.0;


// ============================================================================
//                            SERVO OBJECT
// ============================================================================

Servo myServo;


// ============================================================================
//                         WEB SERVER OBJECTS
// ============================================================================

WebServer server(80);

WebSocketsServer webSocket(81);


// ============================================================================
//                          MOTION VARIABLES
// ============================================================================

float currentRotation = 0.0;

float targetRotation = 0.0;


/*
   Speed percentage.

   1  = very slow
   100 = maximum

   This controls PWM magnitude.
*/

float movementSpeed = 30.0;


bool servoRunning = false;

bool emergencyStopActive = false;


// ============================================================================
//                           TIMING VARIABLES
// ============================================================================

unsigned long lastMotionUpdate = 0;

unsigned long lastStatusUpdate = 0;

unsigned long movementStartTime = 0;


// ============================================================================
//                         DASHBOARD HTML
// ============================================================================

const char MAIN_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,
initial-scale=1.0,
maximum-scale=1.0,
user-scalable=no">

<title>ESP32 Multi-Turn Servo</title>


<style>

/* =========================================================
                         GLOBAL
========================================================= */

* {

    box-sizing: border-box;

    margin: 0;

    padding: 0;

}

body {

    font-family:
        -apple-system,
        BlinkMacSystemFont,
        "SF Pro Display",
        "Segoe UI",
        Roboto,
        Arial,
        sans-serif;

    background:
        linear-gradient(
            180deg,
            #f8f9fb,
            #ffffff 55%,
            #f5f5f7
        );

    color: #1d1d1f;

    min-height: 100vh;

    -webkit-font-smoothing:
        antialiased;
}


/* =========================================================
                         HEADER
========================================================= */

.header {

    position: sticky;

    top: 0;

    z-index: 100;

    background:
        rgba(255,255,255,0.82);

    backdrop-filter:
        blur(20px);

    border-bottom:
        1px solid
        rgba(0,0,0,0.06);

}

.header-inner {

    max-width: 1050px;

    margin: auto;

    padding: 17px 22px;

    display: flex;

    justify-content:
        space-between;

    align-items: center;

}

.brand {

    display: flex;

    align-items: center;

    gap: 11px;

}

.logo {

    width: 40px;

    height: 40px;

    border-radius: 12px;

    background: #1d1d1f;

    color: white;

    display: flex;

    justify-content: center;

    align-items: center;

    font-size: 20px;

}

.brand-title {

    font-size: 16px;

    font-weight: 700;

}

.brand-subtitle {

    font-size: 11px;

    color: #86868b;

    margin-top: 2px;

}


/* =========================================================
                      CONNECTION
========================================================= */

.connection {

    display: flex;

    align-items: center;

    gap: 8px;

    padding: 8px 12px;

    border-radius: 20px;

    background: #f2f2f7;

    font-size: 12px;

    color: #666;

}

.connection-dot {

    width: 8px;

    height: 8px;

    border-radius: 50%;

    background: #ff3b30;

}

.connection-dot.connected {

    background: #34c759;

    box-shadow:
        0 0 10px
        rgba(52,199,89,0.5);

}


/* =========================================================
                          MAIN
========================================================= */

.container {

    max-width: 850px;

    margin: auto;

    padding:
        45px 20px 70px;

}

.hero {

    text-align: center;

    margin-bottom: 38px;

}

.hero h1 {

    font-size:
        clamp(38px,8vw,62px);

    letter-spacing:
        -3px;

}

.hero h1 span {

    color: #86868b;

}

.hero p {

    margin-top: 14px;

    color: #86868b;

}


/* =========================================================
                          CARD
========================================================= */

.card {

    background:
        rgba(255,255,255,0.92);

    border:
        1px solid
        rgba(0,0,0,0.06);

    border-radius: 30px;

    padding: 30px;

    box-shadow:
        0 15px 50px
        rgba(0,0,0,0.055);

}


/* =========================================================
                          DIAL
========================================================= */

.dial {

    width: 230px;

    height: 230px;

    margin:
        5px auto 25px;

    border-radius: 50%;

    position: relative;

    display: flex;

    justify-content: center;

    align-items: center;

    background:
        conic-gradient(
            #1d1d1f
            var(--progress),
            #e8e8ed
            var(--progress)
        );

}

.dial::before {

    content: "";

    position: absolute;

    width: 190px;

    height: 190px;

    border-radius: 50%;

    background: white;

}

.dial-content {

    position: relative;

    z-index: 2;

    text-align: center;

}

.angle {

    font-size: 38px;

    font-weight: 700;

}

.angle-label {

    font-size: 12px;

    color: #86868b;

    margin-top: 5px;

}


/* =========================================================
                        ROTATION INFO
========================================================= */

.rotation-info {

    text-align: center;

    margin-bottom: 25px;

}

.rotation-count {

    font-size: 18px;

    font-weight: 650;

}

.direction {

    font-size: 12px;

    color: #86868b;

    margin-top: 5px;

}


/* =========================================================
                       STATUS GRID
========================================================= */

.status-grid {

    display: grid;

    grid-template-columns:
        repeat(3,1fr);

    gap: 12px;

}

.status-box {

    background: #f7f7f9;

    border-radius: 18px;

    padding: 17px;

}

.status-label {

    font-size: 11px;

    color: #86868b;

    margin-bottom: 6px;

}

.status-value {

    font-size: 18px;

    font-weight: 650;

}


/* =========================================================
                         CONTROL
========================================================= */

.control {

    margin-top: 25px;

}

.control-header {

    display: flex;

    justify-content:
        space-between;

    margin-bottom: 11px;

}

.control-title {

    font-size: 14px;

    font-weight: 650;

}

.control-value {

    color: #86868b;

    font-size: 13px;

}

input[type="range"] {

    width: 100%;

    height: 5px;

    appearance: none;

    background: #dedee3;

    border-radius: 10px;

}

input[type="range"]::-webkit-slider-thumb {

    appearance: none;

    width: 24px;

    height: 24px;

    border-radius: 50%;

    background: white;

    border:
        1px solid #d0d0d5;

    box-shadow:
        0 3px 9px
        rgba(0,0,0,0.18);

}

.number-input {

    width: 100%;

    margin-top: 12px;

    padding: 13px;

    border:
        1px solid #dedee3;

    border-radius: 13px;

    background: #f8f8fa;

    font-size: 15px;

    outline: none;

}


/* =========================================================
                         BUTTONS
========================================================= */

.buttons {

    display: grid;

    grid-template-columns:
        repeat(3,1fr);

    gap: 11px;

    margin-top: 28px;

}

button {

    border: none;

    border-radius: 15px;

    padding: 15px;

    font-size: 14px;

    font-weight: 650;

    cursor: pointer;

}

.start {

    background: #1d1d1f;

    color: white;

}

.stop,
.home {

    background: #f2f2f7;

    color: #1d1d1f;

}

.emergency {

    width: 100%;

    margin-top: 12px;

    background: #ff3b30;

    color: white;

}


/* =========================================================
                         FOOTER
========================================================= */

.footer {

    text-align: center;

    color: #86868b;

    font-size: 11px;

    margin-top: 35px;

}


/* =========================================================
                          TOAST
========================================================= */

.toast {

    position: fixed;

    left: 50%;

    bottom: 25px;

    transform:
        translate(-50%,20px);

    background: #1d1d1f;

    color: white;

    padding: 12px 18px;

    border-radius: 20px;

    font-size: 12px;

    opacity: 0;

    transition:
        all .3s ease;

}

.toast.show {

    opacity: 1;

    transform:
        translate(-50%,0);

}


/* =========================================================
                          MOBILE
========================================================= */

@media(max-width:600px) {

    .container {

        padding:
            32px 15px 55px;

    }

    .card {

        padding: 22px;

    }

    .status-value {

        font-size: 14px;

    }

    .buttons {

        grid-template-columns: 1fr;

    }

    .brand-subtitle {

        display: none;

    }

    .dial {

        width: 190px;

        height: 190px;

    }

    .dial::before {

        width: 157px;

        height: 157px;

    }

}

</style>

</head>


<body>


<header class="header">

<div class="header-inner">

<div class="brand">

<div class="logo">
↻
</div>

<div>

<div class="brand-title">
Multi-Turn Servo
</div>

<div class="brand-subtitle">
ESP32 Rotation Controller
</div>

</div>

</div>


<div class="connection">

<div
id="connectionDot"
class="connection-dot">
</div>

<span id="connectionText">
Connecting
</span>

</div>

</div>

</header>


<main class="container">


<section class="hero">

<h1>
Multi-Turn
<span>Motion.</span>
</h1>

<p>
Control up to ±36000° of accumulated rotation.
</p>

</section>


<section class="card">


<div
id="dial"
class="dial"
style="--progress:0deg;">

<div class="dial-content">

<div
id="angleDisplay"
class="angle">

0°

</div>

<div class="angle-label">
Current Rotation
</div>

</div>

</div>


<div class="rotation-info">

<div
id="rotationCount"
class="rotation-count">

0.00 rotations

</div>

<div
id="direction"
class="direction">

● Stationary

</div>

</div>


<div class="status-grid">


<div class="status-box">

<div class="status-label">
CURRENT
</div>

<div class="status-value">

<span id="currentStatus">
0
</span>°

</div>

</div>


<div class="status-box">

<div class="status-label">
TARGET
</div>

<div class="status-value">

<span id="targetStatus">
0
</span>°

</div>

</div>


<div class="status-box">

<div class="status-label">
STATUS
</div>

<div
id="modeStatus"
class="status-value">

IDLE

</div>

</div>

</div>


<div class="control">

<div class="control-header">

<span class="control-title">
Target Rotation
</span>

<span
id="targetValue"
class="control-value">

0°

</span>

</div>


<input
id="angleSlider"
type="range"
min="-36000"
max="36000"
step="360"
value="0"
oninput="sliderChanged(this.value)">


<input
id="angleInput"
class="number-input"
type="number"
min="-36000"
max="36000"
step="1"
value="0"
onchange="inputChanged(this.value)">

</div>


<div class="control">

<div class="control-header">

<span class="control-title">
Rotation Speed
</span>

<span
id="speedValue"
class="control-value">

30%

</span>

</div>


<input
id="speedSlider"
type="range"
min="1"
max="100"
value="30"
oninput="speedChanged(this.value)">

</div>


<div class="buttons">

<button
class="start"
onclick="startServo()">

▶ Start

</button>


<button
class="stop"
onclick="stopServo()">

■ Stop

</button>


<button
class="home"
onclick="homeServo()">

⌂ Home

</button>

</div>


<button
class="emergency"
onclick="emergencyStop()">

⚠ Emergency Stop

</button>


</section>


<div class="footer">

ESP32 · 360° Continuous Servo · Multi-Turn Controller

</div>


</main>


<div
id="toast"
class="toast">

Command sent

</div>


<script>


// =========================================================
//                       WEBSOCKET
// =========================================================

let socket;

let reconnectTimer;


function connectWebSocket() {

    const host =
        window.location.hostname;


    socket =
        new WebSocket(
            "ws://" +
            host +
            ":81/"
        );


    socket.onopen =
        function() {

            setConnection(true);

            showToast(
                "Connected to ESP32"
            );

        };


    socket.onclose =
        function() {

            setConnection(false);

            clearTimeout(
                reconnectTimer
            );

            reconnectTimer =
                setTimeout(
                    connectWebSocket,
                    2000
                );

        };


    socket.onerror =
        function() {

            setConnection(false);

        };


    socket.onmessage =
        function(event) {

            try {

                updateDashboard(
                    JSON.parse(
                        event.data
                    )
                );

            }
            catch(e) {

                console.log(e);

            }

        };

}


// =========================================================
//                     CONNECTION
// =========================================================

function setConnection(
    connected
) {

    const dot =
        document.getElementById(
            "connectionDot"
        );

    const text =
        document.getElementById(
            "connectionText"
        );


    if(connected) {

        dot.classList.add(
            "connected"
        );

        text.innerText =
            "Connected";

    }
    else {

        dot.classList.remove(
            "connected"
        );

        text.innerText =
            "Disconnected";

    }

}


// =========================================================
//                    SEND COMMAND
// =========================================================

function sendCommand(
    command
) {

    if(
        socket &&
        socket.readyState ===
        WebSocket.OPEN
    ) {

        socket.send(
            command
        );

    }
    else {

        showToast(
            "ESP32 not connected"
        );

    }

}


// =========================================================
//                     TARGET SLIDER
// =========================================================

function sliderChanged(
    value
) {

    value =
        clamp(
            Number(value),
            -36000,
            36000
        );


    document.getElementById(
        "angleInput"
    ).value =
        value;


    document.getElementById(
        "targetValue"
    ).innerText =
        formatAngle(value);


    sendTarget(
        value
    );

}


// =========================================================
//                     TARGET INPUT
// =========================================================

function inputChanged(
    value
) {

    value =
        Number(value);


    if(
        !Number.isFinite(value)
    ) {

        value = 0;

    }


    value =
        clamp(
            value,
            -36000,
            36000
        );


    document.getElementById(
        "angleInput"
    ).value =
        value;


    document.getElementById(
        "angleSlider"
    ).value =
        value;


    document.getElementById(
        "targetValue"
    ).innerText =
        formatAngle(value);


    sendTarget(
        value
    );

}


function sendTarget(
    value
) {

    sendCommand(
        JSON.stringify({

            type:
                "target",

            value:
                value

        })
    );

}


// =========================================================
//                         SPEED
// =========================================================

function speedChanged(
    value
) {

    value =
        clamp(
            Number(value),
            1,
            100
        );


    document.getElementById(
        "speedValue"
    ).innerText =
        value +
        "%";


    sendCommand(
        JSON.stringify({

            type:
                "speed",

            value:
                value

        })
    );

}


// =========================================================
//                         BUTTONS
// =========================================================

function startServo() {

    sendCommand(
        JSON.stringify({
            type:"start"
        })
    );

    showToast(
        "Rotation started"
    );

}


function stopServo() {

    sendCommand(
        JSON.stringify({
            type:"stop"
        })
    );

    showToast(
        "Rotation stopped"
    );

}


function homeServo() {

    sendCommand(
        JSON.stringify({
            type:"home"
        })
    );

    showToast(
        "Returning to zero"
    );

}


function emergencyStop() {

    sendCommand(
        JSON.stringify({
            type:"emergency"
        })
    );

    showToast(
        "EMERGENCY STOP"
    );

}


// =========================================================
//                    DASHBOARD UPDATE
// =========================================================

function updateDashboard(
    data
) {

    if(
        data.current !==
        undefined
    ) {

        const current =
            Number(
                data.current
            );


        document.getElementById(
            "currentStatus"
        ).innerText =
            current.toFixed(0);


        document.getElementById(
            "angleDisplay"
        ).innerText =
            formatAngle(current);


        updateRotation(
            current
        );

    }


    if(
        data.target !==
        undefined
    ) {

        document.getElementById(
            "targetStatus"
        ).innerText =
            Number(
                data.target
            ).toFixed(0);

    }


    if(
        data.mode !==
        undefined
    ) {

        document.getElementById(
            "modeStatus"
        ).innerText =
            data.mode;

    }


    if(
        data.speed !==
        undefined
    ) {

        document.getElementById(
            "speedValue"
        ).innerText =
            Number(
                data.speed
            ).toFixed(0) +
            "%";

    }

}


// =========================================================
//                   ROTATION DISPLAY
// =========================================================

function updateRotation(
    angle
) {

    const rotations =
        angle / 360;


    document.getElementById(
        "rotationCount"
    ).innerText =
        rotations.toFixed(2) +
        " rotations";


    const direction =
        document.getElementById(
            "direction"
        );


    if(angle > 0.1) {

        direction.innerText =
            "↻ Clockwise";

    }

    else if(angle < -0.1) {

        direction.innerText =
            "↺ Counter-clockwise";

    }

    else {

        direction.innerText =
            "● Zero Position";

    }


    let dial =
        ((angle % 360) + 360) % 360;


    document.getElementById(
        "dial"
    ).style.setProperty(
        "--progress",
        dial + "deg"
    );

}


// =========================================================
//                         HELPERS
// =========================================================

function clamp(
    value,
    min,
    max
) {

    return Math.min(
        Math.max(
            value,
            min
        ),
        max
    );

}


function formatAngle(
    angle
) {

    angle =
        Number(angle);


    if(angle > 0) {

        return "+" +
            angle.toFixed(0) +
            "°";

    }


    return angle.toFixed(0) +
        "°";

}


// =========================================================
//                          TOAST
// =========================================================

let toastTimer;


function showToast(
    message
) {

    const toast =
        document.getElementById(
            "toast"
        );


    toast.innerText =
        message;


    toast.classList.add(
        "show"
    );


    clearTimeout(
        toastTimer
    );


    toastTimer =
        setTimeout(
            function() {

                toast.classList.remove(
                    "show"
                );

            },
            1800
        );

}


// =========================================================
//                       CONNECT
// =========================================================

connectWebSocket();

</script>


</body>

</html>

)rawliteral";


// ============================================================================
//                    SERVO STOP
// ============================================================================

void stopServoOutput()
{

    myServo.writeMicroseconds(
        SERVO_STOP_US
    );

}


// ============================================================================
//                    CALCULATE SERVO PWM
// ============================================================================

int calculatePWM()
{

    /*
       Convert speed percentage to PWM.

       Positive target difference:
           Clockwise

       Negative target difference:
           Counter-clockwise
    */

    float difference =
        targetRotation -
        currentRotation;


    if(
        difference > 0
    )
    {

        int pwm =
            map(
                (int)movementSpeed,
                1,
                100,
                SERVO_CW_MIN_US,
                SERVO_CW_MAX_US
            );

        return pwm;

    }


    if(
        difference < 0
    )
    {

        int pwm =
            map(
                (int)movementSpeed,
                1,
                100,
                SERVO_CCW_MIN_US,
                SERVO_CCW_MAX_US
            );

        return pwm;

    }


    return SERVO_STOP_US;

}


// ============================================================================
//                         SEND STATUS
// ============================================================================

void sendStatus(
    uint8_t client = 255
)
{

    String mode;


    if(
        emergencyStopActive
    )
    {

        mode =
            "E-STOP";

    }

    else if(
        servoRunning
    )
    {

        mode =
            "RUNNING";

    }

    else
    {

        mode =
            "IDLE";

    }


    String json = "{";


    json +=
        "\"current\":" +
        String(
            currentRotation,
            1
        ) +
        ",";


    json +=
        "\"target\":" +
        String(
            targetRotation,
            1
        ) +
        ",";


    json +=
        "\"speed\":" +
        String(
            movementSpeed,
            0
        ) +
        ",";


    json +=
        "\"mode\":\"" +
        mode +
        "\"";


    json += "}";


    if(
        client == 255
    )
    {

        webSocket.broadcastTXT(
            json
        );

    }

    else
    {

        webSocket.sendTXT(
            client,
            json
        );

    }

}


// ============================================================================
//                      WEBSOCKET EVENT
// ============================================================================

void webSocketEvent(
    uint8_t client,
    WStype_t type,
    uint8_t *payload,
    size_t length
)
{

    if(
        type ==
        WStype_CONNECTED
    )
    {

        Serial.print(
            "WebSocket client connected: "
        );

        Serial.println(
            client
        );


        sendStatus(
            client
        );

        return;

    }


    if(
        type !=
        WStype_TEXT
    )
    {

        return;

    }


    String message =
        String(
            (char*)payload
        );


    Serial.print(
        "Command: "
    );

    Serial.println(
        message
    );


    // ========================================================
    // TARGET
    // ========================================================

    if(
        message.indexOf(
            "\"type\":\"target\""
        ) >= 0
    )
    {

        int position =
            message.indexOf(
                "\"value\":"
            );


        if(
            position >= 0
        )
        {

            String value =
                message.substring(
                    position + 8
                );


            float angle =
                value.toFloat();


            targetRotation =
                constrain(
                    angle,
                    MIN_ROTATION,
                    MAX_ROTATION
                );


            Serial.print(
                "Target = "
            );

            Serial.print(
                targetRotation
            );

            Serial.println(
                "°"
            );

        }

    }


    // ========================================================
    // SPEED
    // ========================================================

    else if(
        message.indexOf(
            "\"type\":\"speed\""
        ) >= 0
    )
    {

        int position =
            message.indexOf(
                "\"value\":"
            );


        if(
            position >= 0
        )
        {

            String value =
                message.substring(
                    position + 8
                );


            movementSpeed =
                value.toFloat();


            movementSpeed =
                constrain(
                    movementSpeed,
                    1.0,
                    100.0
                );

        }

    }


    // ========================================================
    // START
    // ========================================================

    else if(
        message.indexOf(
            "\"type\":\"start\""
        ) >= 0
    )
    {

        emergencyStopActive =
            false;


        if(
            currentRotation !=
            targetRotation
        )
        {

            servoRunning =
                true;

            lastMotionUpdate =
                millis();

            Serial.println(
                "ROTATION STARTED"
            );

        }

    }


    // ========================================================
    // STOP
    // ========================================================

    else if(
        message.indexOf(
            "\"type\":\"stop\""
        ) >= 0
    )
    {

        servoRunning =
            false;


        stopServoOutput();


        Serial.println(
            "ROTATION STOPPED"
        );

    }


    // ========================================================
    // HOME
    // ========================================================

    else if(
        message.indexOf(
            "\"type\":\"home\""
        ) >= 0
    )
    {

        emergencyStopActive =
            false;

        targetRotation =
            0.0;

        servoRunning =
            true;

        lastMotionUpdate =
            millis();


        Serial.println(
            "RETURNING TO ZERO"
        );

    }


    // ========================================================
    // EMERGENCY
    // ========================================================

    else if(
        message.indexOf(
            "\"type\":\"emergency\""
        ) >= 0
    )
    {

        servoRunning =
            false;

        emergencyStopActive =
            true;


        stopServoOutput();


        Serial.println(
            "!!! EMERGENCY STOP !!!"
        );

    }


    sendStatus();

}


// ============================================================================
//                         ROOT PAGE
// ============================================================================

void handleRoot()
{

    server.send_P(
        200,
        "text/html",
        MAIN_PAGE
    );

}


// ============================================================================
//                         WIFI
// ============================================================================

void connectWiFi()
{

    WiFi.mode(
        WIFI_STA
    );


    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    Serial.println();

    Serial.println(
        "========================================"
    );

    Serial.println(
        "Connecting to Wi-Fi..."
    );


    unsigned long start =
        millis();


    while(
        WiFi.status() !=
        WL_CONNECTED
    )
    {

        delay(500);

        Serial.print(
            "."
        );


        if(
            millis() -
            start >
            20000
        )
        {

            Serial.println();

            Serial.println(
                "Wi-Fi connection failed."
            );

            ESP.restart();

        }

    }


    Serial.println();

    Serial.println(
        "Wi-Fi CONNECTED"
    );


    Serial.print(
        "IP Address: "
    );

    Serial.println(
        WiFi.localIP()
    );


    Serial.print(
        "RSSI: "
    );

    Serial.print(
        WiFi.RSSI()
    );

    Serial.println(
        " dBm"
    );

}


// ============================================================================
//                        MOTION UPDATE
// ============================================================================

void updateRotation()
{

    if(
        !servoRunning ||
        emergencyStopActive
    )
    {

        return;

    }


    unsigned long now =
        millis();


    unsigned long elapsed =
        now -
        lastMotionUpdate;


    if(
        elapsed < 10
    )
    {

        return;

    }


    lastMotionUpdate =
        now;


    float seconds =
        elapsed /
        1000.0;


    /*
       Determine direction.
    */

    float difference =
        targetRotation -
        currentRotation;


    if(
        fabs(difference) < 0.5
    )
    {

        currentRotation =
            targetRotation;


        servoRunning =
            false;


        stopServoOutput();


        Serial.println(
            "TARGET REACHED"
        );


        sendStatus();

        return;

    }


    /*
       Calculate estimated movement.

       Maximum calibrated speed is
       degreesPerSecondAtMaxSpeed.

       Speed percentage scales this value.
    */

    float speed =
        degreesPerSecondAtMaxSpeed *
        (movementSpeed / 100.0);


    float movement =
        speed *
        seconds;


    /*
       Prevent overshoot.
    */

    if(
        fabs(difference) <=
        movement
    )
    {

        currentRotation =
            targetRotation;

    }

    else if(
        difference > 0
    )
    {

        currentRotation +=
            movement;

    }

    else
    {

        currentRotation -=
            movement;

    }


    currentRotation =
        constrain(
            currentRotation,
            MIN_ROTATION,
            MAX_ROTATION
        );


    /*
       Set servo direction and speed.
    */

    int pwm =
        calculatePWM();


    myServo.writeMicroseconds(
        pwm
    );

}


// ============================================================================
//                              SETUP
// ============================================================================

void setup()
{

    Serial.begin(
        115200
    );


    delay(1000);


    Serial.println();

    Serial.println(
        "========================================"
    );

    Serial.println(
        " ESP32 360° CONTINUOUS SERVO CONTROLLER"
    );

    Serial.println(
        "========================================"
    );


    // --------------------------------------------------------
    // SERVO
    // --------------------------------------------------------

    myServo.setPeriodHertz(
        50
    );


    myServo.attach(
        SERVO_PIN,
        1000,
        2000
    );


    stopServoOutput();


    currentRotation =
        0.0;

    targetRotation =
        0.0;


    Serial.print(
        "Servo GPIO: "
    );

    Serial.println(
        SERVO_PIN
    );


    Serial.println(
        "Rotation range: -36000° to +36000°"
    );


    Serial.print(
        "Calibration speed: "
    );

    Serial.print(
        degreesPerSecondAtMaxSpeed
    );

    Serial.println(
        "°/second"
    );


    // --------------------------------------------------------
    // WIFI
    // --------------------------------------------------------

    connectWiFi();


    // --------------------------------------------------------
    // HTTP SERVER
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );


    server.begin();


    Serial.println(
        "HTTP server started"
    );


    // --------------------------------------------------------
    // WEBSOCKET
    // --------------------------------------------------------

    webSocket.begin();

    webSocket.onEvent(
        webSocketEvent
    );


    Serial.println(
        "WebSocket server started"
    );


    Serial.println();

    Serial.print(
        "Open: http://"
    );

    Serial.println(
        WiFi.localIP()
    );

    Serial.println();

}


// ============================================================================
//                               LOOP
// ============================================================================

void loop()
{

    server.handleClient();

    webSocket.loop();

    updateRotation();


    if(
        millis() -
        lastStatusUpdate >=
        100
    )
    {

        lastStatusUpdate =
            millis();

        sendStatus();

    }

}