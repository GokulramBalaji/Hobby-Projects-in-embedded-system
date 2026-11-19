/*
===============================================================================
                      ESP32 4-GEAR AMT CONTROLLER  (FIXED)
===============================================================================
Board   : ESP32 Dev Module
Library : ESP32Servo (install from Library Manager)
Wi-Fi   : SSID "Wifi"  /  password "aaaaaaac"  ->  http://192.168.4.1

Bench-test first. Power the servos / relay / stepper driver from a SEPARATE
supply (common GND with the ESP32). Servos powered from the ESP32 board cause
brown-out resets, and then the Wi-Fi never appears.
===============================================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESP32Servo.h>

// ----------------------------- PINS ----------------------------------------
#define CLUTCH_SERVO_PIN     18
#define RPM_SERVO_PIN        23
#define ENGINE_RELAY_PIN     19
#define STEPPER_STEP_PIN     25
#define STEPPER_DIR_PIN      26
#define STEPPER_ENABLE_PIN   27

// ------------------------- HARDWARE LOGIC ----------------------------------
#define ENGINE_ON_LEVEL        HIGH
#define ENGINE_OFF_LEVEL       LOW
#define STEPPER_ENABLE_LEVEL   LOW
#define STEPPER_DISABLE_LEVEL  HIGH

// ------------------------- FACTORY DEFAULTS --------------------------------
const float DEFAULT_GEAR_1_SPEED = 15.0f;
const float DEFAULT_GEAR_2_SPEED = 30.0f;
const float DEFAULT_GEAR_3_SPEED = 50.0f;
const float DEFAULT_GEAR_4_SPEED = 75.0f;
const float DEFAULT_DOWNSHIFT_HYSTERESIS = 5.0f;

const int DEFAULT_CLUTCH_NORMAL_ANGLE = 20;
const int DEFAULT_CLUTCH_PULL_ANGLE   = 100;
const int DEFAULT_RPM_LOW_ANGLE       = 0;
const int DEFAULT_RPM_MAX_ANGLE       = 90;

const float DEFAULT_UPSHIFT_ANGLE   = 30.0f;
const float DEFAULT_DOWNSHIFT_ANGLE = 30.0f;
const float DEFAULT_STEPS_PER_REV   = 3200.0f;
const float DEFAULT_STEPPER_SPEED   = 800.0f;

const uint32_t DEFAULT_CLUTCH_PULL_DELAY    = 300;
const uint32_t DEFAULT_ENGINE_CUT_DELAY     = 100;
const uint32_t DEFAULT_GEAR_SHIFT_DELAY     = 100;
const uint32_t DEFAULT_ENGINE_RESTORE_DELAY = 200;
const uint32_t DEFAULT_CLUTCH_RELEASE_DELAY = 300;
const uint32_t DEFAULT_SHIFT_INTERVAL       = 4000;

const float DEFAULT_SIMULATION_SPEED     = 0.0f;
const float DEFAULT_SIMULATION_MAX_SPEED = 75.0f;

// ------------------------------ WIFI ---------------------------------------
const char* AP_NAME     = "Wifi";
const char* AP_PASSWORD = "aaaaaaac";

// ----------------------------- OBJECTS -------------------------------------
WebServer   server(80);
Preferences preferences;
Servo       clutchServo;
Servo       rpmServo;

struct AMTConfig {
  float gear1Speed, gear2Speed, gear3Speed, gear4Speed;
  float downshiftHysteresis;
  int   clutchNormalAngle, clutchPullAngle;
  int   rpmLowAngle, rpmMaxAngle;
  float upshiftAngle, downshiftAngle;
  float stepsPerRevolution, stepperSpeed;
  uint32_t clutchPullDelay, engineCutDelay, gearShiftDelay;
  uint32_t engineRestoreDelay, clutchReleaseDelay, shiftInterval;
  float simulationSpeed, maxSimulationSpeed;
};
AMTConfig config;

// --------------------------- RUNTIME STATE ---------------------------------
int   currentGear   = 0;      // 0 = Neutral
int   requestedGear = 0;
int   manualTarget  = 0;
float simulatedSpeed = 0.0f;
bool  automaticMode = true;
bool  emergencyStop = false;

enum ClutchState { CLUTCH_NORMAL, CLUTCH_PULLED };
ClutchState clutchState = CLUTCH_NORMAL;

bool engineRunning = true;
int  rpmAngleNow   = 0;
volatile bool stepperBusy = false;

enum AMTState {
  AMT_IDLE, AMT_CLUTCH_PULL, AMT_RPM_REDUCE, AMT_ENGINE_CUT, AMT_GEAR_SHIFT,
  AMT_STEPPER_RETURN, AMT_ENGINE_RESTORE, AMT_RPM_RESTORE, AMT_CLUTCH_RELEASE,
  AMT_COMPLETE, AMT_FAULT
};
AMTState amtState = AMT_IDLE;

uint32_t stateStartedAt = 0;
uint32_t lastShiftCompletedAt = 0;

long     stepperTargetSteps = 0;
long     stepperCompletedSteps = 0;
int      stepperDirection = 1;
uint32_t lastStepMicros = 0;
float    currentStepperAngle = 0.0f;
float    targetStepperAngle  = 0.0f;

int  shiftFromGear = 0;
int  shiftToGear = 0;
bool shiftIsUpshift = false;

String lastAction = "READY";

// ---------------------------- HELPERS --------------------------------------
float clampF(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
long  clampL(long v, long lo, long hi)    { return v < lo ? lo : (v > hi ? hi : v); }

String gearName(int gear) { return gear == 0 ? String("N") : String(gear); }

String stateName() {
  switch (amtState) {
    case AMT_IDLE:            return "READY";
    case AMT_CLUTCH_PULL:     return "CLUTCH PULL";
    case AMT_RPM_REDUCE:      return "RPM REDUCE";
    case AMT_ENGINE_CUT:      return "ENGINE CUT";
    case AMT_GEAR_SHIFT:      return "GEAR SHIFT";
    case AMT_STEPPER_RETURN:  return "STEPPER RETURN";
    case AMT_ENGINE_RESTORE:  return "ENGINE RESTORE";
    case AMT_RPM_RESTORE:     return "RPM RESTORE";
    case AMT_CLUTCH_RELEASE:  return "CLUTCH RELEASE";
    case AMT_COMPLETE:        return "COMPLETE";
    case AMT_FAULT:           return "FAULT";
  }
  return "UNKNOWN";
}

// --------------------------- CONFIG LIMITS ---------------------------------
void sanitizeConfig() {
  config.gear1Speed = clampF(config.gear1Speed, 1.0f, 300.0f);
  config.gear2Speed = clampF(config.gear2Speed, config.gear1Speed + 0.1f, 300.0f);
  config.gear3Speed = clampF(config.gear3Speed, config.gear2Speed + 0.1f, 300.0f);
  config.gear4Speed = clampF(config.gear4Speed, config.gear3Speed + 0.1f, 300.0f);
  config.downshiftHysteresis = clampF(config.downshiftHysteresis, 0.0f, 20.0f);

  config.clutchNormalAngle = (int)clampL(config.clutchNormalAngle, 0, 180);
  config.clutchPullAngle   = (int)clampL(config.clutchPullAngle, 0, 180);
  config.rpmLowAngle       = (int)clampL(config.rpmLowAngle, 0, 179);
  config.rpmMaxAngle       = (int)clampL(config.rpmMaxAngle, config.rpmLowAngle + 1, 180);

  config.upshiftAngle   = clampF(config.upshiftAngle, 0.1f, 180.0f);
  config.downshiftAngle = clampF(config.downshiftAngle, 0.1f, 180.0f);
  config.stepsPerRevolution = clampF(config.stepsPerRevolution, 1.0f, 200000.0f);
  config.stepperSpeed       = clampF(config.stepperSpeed, 10.0f, 10000.0f);

  config.clutchPullDelay    = (uint32_t)clampL((long)config.clutchPullDelay, 0, 10000);
  config.engineCutDelay     = (uint32_t)clampL((long)config.engineCutDelay, 0, 10000);
  config.gearShiftDelay     = (uint32_t)clampL((long)config.gearShiftDelay, 0, 10000);
  config.engineRestoreDelay = (uint32_t)clampL((long)config.engineRestoreDelay, 0, 10000);
  config.clutchReleaseDelay = (uint32_t)clampL((long)config.clutchReleaseDelay, 0, 10000);
  config.shiftInterval      = (uint32_t)clampL((long)config.shiftInterval, 500, 60000);

  config.maxSimulationSpeed = clampF(config.maxSimulationSpeed, 1.0f, 300.0f);
  config.simulationSpeed    = clampF(config.simulationSpeed, 0.0f, config.maxSimulationSpeed);
}

void loadDefaultConfig() {
  config.gear1Speed = DEFAULT_GEAR_1_SPEED;
  config.gear2Speed = DEFAULT_GEAR_2_SPEED;
  config.gear3Speed = DEFAULT_GEAR_3_SPEED;
  config.gear4Speed = DEFAULT_GEAR_4_SPEED;
  config.downshiftHysteresis = DEFAULT_DOWNSHIFT_HYSTERESIS;
  config.clutchNormalAngle = DEFAULT_CLUTCH_NORMAL_ANGLE;
  config.clutchPullAngle   = DEFAULT_CLUTCH_PULL_ANGLE;
  config.rpmLowAngle       = DEFAULT_RPM_LOW_ANGLE;
  config.rpmMaxAngle       = DEFAULT_RPM_MAX_ANGLE;
  config.upshiftAngle   = DEFAULT_UPSHIFT_ANGLE;
  config.downshiftAngle = DEFAULT_DOWNSHIFT_ANGLE;
  config.stepsPerRevolution = DEFAULT_STEPS_PER_REV;
  config.stepperSpeed       = DEFAULT_STEPPER_SPEED;
  config.clutchPullDelay    = DEFAULT_CLUTCH_PULL_DELAY;
  config.engineCutDelay     = DEFAULT_ENGINE_CUT_DELAY;
  config.gearShiftDelay     = DEFAULT_GEAR_SHIFT_DELAY;
  config.engineRestoreDelay = DEFAULT_ENGINE_RESTORE_DELAY;
  config.clutchReleaseDelay = DEFAULT_CLUTCH_RELEASE_DELAY;
  config.shiftInterval      = DEFAULT_SHIFT_INTERVAL;
  config.simulationSpeed    = DEFAULT_SIMULATION_SPEED;
  config.maxSimulationSpeed = DEFAULT_SIMULATION_MAX_SPEED;
  sanitizeConfig();
}

void saveConfig() {
  preferences.begin("amtcfg", false);
  preferences.putFloat("g1", config.gear1Speed);
  preferences.putFloat("g2", config.gear2Speed);
  preferences.putFloat("g3", config.gear3Speed);
  preferences.putFloat("g4", config.gear4Speed);
  preferences.putFloat("hyst", config.downshiftHysteresis);
  preferences.putInt("clNorm", config.clutchNormalAngle);
  preferences.putInt("clPull", config.clutchPullAngle);
  preferences.putInt("rpmLow", config.rpmLowAngle);
  preferences.putInt("rpmMax", config.rpmMaxAngle);
  preferences.putFloat("upAng", config.upshiftAngle);
  preferences.putFloat("dnAng", config.downshiftAngle);
  preferences.putFloat("stepsRev", config.stepsPerRevolution);
  preferences.putFloat("stepSpeed", config.stepperSpeed);
  preferences.putULong("clPullD", config.clutchPullDelay);
  preferences.putULong("engCutD", config.engineCutDelay);
  preferences.putULong("gearD", config.gearShiftDelay);
  preferences.putULong("engRestD", config.engineRestoreDelay);
  preferences.putULong("clRelD", config.clutchReleaseDelay);
  preferences.putULong("shiftInt", config.shiftInterval);
  preferences.putFloat("simSpeed", config.simulationSpeed);
  preferences.putFloat("maxSpeed", config.maxSimulationSpeed);
  preferences.end();
  Serial.println("Configuration saved.");
}

void loadConfig() {
  loadDefaultConfig();
  preferences.begin("amtcfg", false);   // read/write mode so the namespace is created on first boot
  config.gear1Speed = preferences.getFloat("g1", config.gear1Speed);
  config.gear2Speed = preferences.getFloat("g2", config.gear2Speed);
  config.gear3Speed = preferences.getFloat("g3", config.gear3Speed);
  config.gear4Speed = preferences.getFloat("g4", config.gear4Speed);
  config.downshiftHysteresis = preferences.getFloat("hyst", config.downshiftHysteresis);
  config.clutchNormalAngle = preferences.getInt("clNorm", config.clutchNormalAngle);
  config.clutchPullAngle   = preferences.getInt("clPull", config.clutchPullAngle);
  config.rpmLowAngle       = preferences.getInt("rpmLow", config.rpmLowAngle);
  config.rpmMaxAngle       = preferences.getInt("rpmMax", config.rpmMaxAngle);
  config.upshiftAngle   = preferences.getFloat("upAng", config.upshiftAngle);
  config.downshiftAngle = preferences.getFloat("dnAng", config.downshiftAngle);
  config.stepsPerRevolution = preferences.getFloat("stepsRev", config.stepsPerRevolution);
  config.stepperSpeed       = preferences.getFloat("stepSpeed", config.stepperSpeed);
  config.clutchPullDelay    = preferences.getULong("clPullD", config.clutchPullDelay);
  config.engineCutDelay     = preferences.getULong("engCutD", config.engineCutDelay);
  config.gearShiftDelay     = preferences.getULong("gearD", config.gearShiftDelay);
  config.engineRestoreDelay = preferences.getULong("engRestD", config.engineRestoreDelay);
  config.clutchReleaseDelay = preferences.getULong("clRelD", config.clutchReleaseDelay);
  config.shiftInterval      = preferences.getULong("shiftInt", config.shiftInterval);
  config.simulationSpeed    = preferences.getFloat("simSpeed", config.simulationSpeed);
  config.maxSimulationSpeed = preferences.getFloat("maxSpeed", config.maxSimulationSpeed);
  preferences.end();
  sanitizeConfig();
}

void factoryReset() {
  preferences.begin("amtcfg", false);
  preferences.clear();
  preferences.end();
  loadDefaultConfig();
  saveConfig();
  simulatedSpeed = config.simulationSpeed;
  Serial.println("Factory configuration restored.");
}

// ---------------------------- ACTUATORS ------------------------------------
void clutchNormal() {
  clutchServo.write(config.clutchNormalAngle);
  clutchState = CLUTCH_NORMAL;
  lastAction = "CLUTCH RELEASED";
}
void clutchPull() {
  clutchServo.write(config.clutchPullAngle);
  clutchState = CLUTCH_PULLED;
  lastAction = "CLUTCH PULLED";
}
void rpmLow() {
  rpmAngleNow = config.rpmLowAngle;
  rpmServo.write(rpmAngleNow);
  lastAction = "RPM REDUCED";
}
void rpmMax() {
  rpmAngleNow = config.rpmMaxAngle;
  rpmServo.write(rpmAngleNow);
  lastAction = "RPM RESTORED";
}
void engineOn() {
  digitalWrite(ENGINE_RELAY_PIN, ENGINE_ON_LEVEL);
  engineRunning = true;
  lastAction = "ENGINE ON";
}
void engineOff() {
  digitalWrite(ENGINE_RELAY_PIN, ENGINE_OFF_LEVEL);
  engineRunning = false;
  lastAction = "ENGINE OFF";
}
void stepperEnable()  { digitalWrite(STEPPER_ENABLE_PIN, STEPPER_ENABLE_LEVEL); }
void stepperDisable() { digitalWrite(STEPPER_ENABLE_PIN, STEPPER_DISABLE_LEVEL); }

void startStepperMove(float angle) {
  if (angle > -0.001f && angle < 0.001f) return;
  targetStepperAngle = currentStepperAngle + angle;
  float stepsFloat = (angle / 360.0f) * config.stepsPerRevolution;
  stepperTargetSteps = labs((long)roundf(stepsFloat));
  stepperCompletedSteps = 0;
  stepperDirection = angle > 0 ? 1 : -1;
  digitalWrite(STEPPER_DIR_PIN, stepperDirection > 0 ? HIGH : LOW);
  stepperEnable();
  delayMicroseconds(20);              // driver DIR/ENABLE setup time
  stepperBusy = true;
  lastStepMicros = micros();
}

void updateStepper() {
  if (!stepperBusy) return;
  if (stepperCompletedSteps >= stepperTargetSteps) {
    stepperBusy = false;
    currentStepperAngle = targetStepperAngle;
    stepperDisable();
    return;
  }
  uint32_t now = micros();
  uint32_t interval = (uint32_t)(1000000.0f / config.stepperSpeed);
  if ((uint32_t)(now - lastStepMicros) < interval) return;
  lastStepMicros = now;
  digitalWrite(STEPPER_STEP_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(STEPPER_STEP_PIN, LOW);
  stepperCompletedSteps++;
}

// -------------------------- GEAR SELECTION ---------------------------------
float gearThreshold(int gear) {
  switch (gear) {
    case 1: return config.gear1Speed;
    case 2: return config.gear2Speed;
    case 3: return config.gear3Speed;
    case 4: return config.gear4Speed;
  }
  return 0.0f;
}

int calculateTargetGear(float speed) {
  if (speed <= 0.0f) return 0;
  if (speed < config.gear1Speed) return 1;
  if (speed < config.gear2Speed) return 2;
  if (speed < config.gear3Speed) return 3;
  return 4;
}

int calculateDownshiftGear(float speed) {
  if (currentGear == 1) return (speed <= 0.0f) ? 0 : 1;   // back to neutral at 0 km/h
  if (currentGear < 2) return currentGear;
  float threshold = gearThreshold(currentGear - 1) - config.downshiftHysteresis;
  if (speed < threshold) return currentGear - 1;
  return currentGear;
}

bool startShift(int targetGear, bool ignoreInterval) {
  if (emergencyStop) return false;
  if (amtState != AMT_IDLE) return false;
  if (targetGear < 0 || targetGear > 4) return false;
  if (targetGear == currentGear) return false;
  if (!ignoreInterval && (millis() - lastShiftCompletedAt < config.shiftInterval)) return false;

  shiftFromGear = currentGear;
  shiftToGear = targetGear;
  requestedGear = targetGear;
  shiftIsUpshift = targetGear > currentGear;

  Serial.printf("SHIFT %s -> %s (%s)\n", gearName(currentGear).c_str(),
                gearName(targetGear).c_str(), shiftIsUpshift ? "UP" : "DOWN");

  clutchPull();
  amtState = AMT_CLUTCH_PULL;
  stateStartedAt = millis();
  lastAction = "SHIFTING";
  return true;
}

void emergencyStopSystem() {
  emergencyStop = true;
  stepperBusy = false;
  stepperDisable();
  engineOff();
  rpmLow();
  clutchPull();
  amtState = AMT_FAULT;
  Serial.println("EMERGENCY STOP");
  lastAction = "EMERGENCY STOP";
}

void clearEmergency() {
  emergencyStop = false;
  currentStepperAngle = 0.0f;
  targetStepperAngle = 0.0f;
  engineOn();
  rpmMax();
  clutchNormal();
  amtState = AMT_IDLE;
  lastShiftCompletedAt = millis();
  lastAction = "READY";
  Serial.println("Emergency cleared.");
}

// -------------------------- AMT STATE MACHINE ------------------------------
void updateAMT() {
  uint32_t now = millis();

  switch (amtState) {

    case AMT_IDLE: {
      if (emergencyStop) return;

      if (!automaticMode) {
        if (manualTarget != currentGear) {
          int next = currentGear + (manualTarget > currentGear ? 1 : -1);
          startShift(next, true);
        }
        return;
      }

      if (currentGear == 0) {
        if (simulatedSpeed > 0.0f) startShift(1, false);
        return;
      }

      if (currentGear < 4 && simulatedSpeed >= gearThreshold(currentGear)) {
        startShift(currentGear + 1, false);
        return;
      }

      int downGear = calculateDownshiftGear(simulatedSpeed);
      if (downGear < currentGear) startShift(downGear, false);
      break;
    }

    case AMT_CLUTCH_PULL:
      if (now - stateStartedAt >= config.clutchPullDelay) {
        rpmLow();
        amtState = AMT_RPM_REDUCE;
        stateStartedAt = now;
        lastAction = "RPM REDUCING";
      }
      break;

    case AMT_RPM_REDUCE:
      if (now - stateStartedAt >= config.engineCutDelay) {
        engineOff();
        amtState = AMT_ENGINE_CUT;
        stateStartedAt = now;
        lastAction = "ENGINE CUT";
      }
      break;

    case AMT_ENGINE_CUT: {
      float angle = shiftIsUpshift ? config.upshiftAngle : -config.downshiftAngle;
      startStepperMove(angle);
      amtState = AMT_GEAR_SHIFT;
      stateStartedAt = now;
      lastAction = shiftIsUpshift ? "UPSHIFT ACTUATOR" : "DOWNSHIFT ACTUATOR";
      break;
    }

    case AMT_GEAR_SHIFT:
      if (stepperBusy) break;
      if (now - stateStartedAt >= config.gearShiftDelay) {
        startStepperMove(-currentStepperAngle);
        amtState = AMT_STEPPER_RETURN;
        stateStartedAt = now;
        lastAction = "RETURNING ACTUATOR";
      }
      break;

    case AMT_STEPPER_RETURN:
      if (!stepperBusy) {
        currentStepperAngle = 0.0f;
        targetStepperAngle = 0.0f;
        engineOn();
        amtState = AMT_ENGINE_RESTORE;
        stateStartedAt = now;
        lastAction = "ENGINE RESTORED";
      }
      break;

    case AMT_ENGINE_RESTORE:
      if (now - stateStartedAt >= config.engineRestoreDelay) {
        rpmMax();
        amtState = AMT_RPM_RESTORE;
        stateStartedAt = now;
        lastAction = "RPM RESTORING";
      }
      break;

    case AMT_RPM_RESTORE:
      clutchNormal();
      amtState = AMT_CLUTCH_RELEASE;
      stateStartedAt = now;
      lastAction = "CLUTCH RELEASE";
      break;

    case AMT_CLUTCH_RELEASE:
      if (now - stateStartedAt >= config.clutchReleaseDelay) {
        currentGear = requestedGear;
        amtState = AMT_COMPLETE;
        stateStartedAt = now;
        lastShiftCompletedAt = now;
        lastAction = "SHIFT COMPLETE";
      }
      break;

    case AMT_COMPLETE:
      if (now - stateStartedAt >= 100) {
        amtState = AMT_IDLE;
        lastAction = "READY";
      }
      break;

    case AMT_FAULT:
      break;
  }
}

// ------------------------------ JSON ---------------------------------------
String jsonConfig() {
  String j = "{";
  j += "\"g1\":" + String(config.gear1Speed, 2);
  j += ",\"g2\":" + String(config.gear2Speed, 2);
  j += ",\"g3\":" + String(config.gear3Speed, 2);
  j += ",\"g4\":" + String(config.gear4Speed, 2);
  j += ",\"hyst\":" + String(config.downshiftHysteresis, 2);
  j += ",\"clNormal\":" + String(config.clutchNormalAngle);
  j += ",\"clPull\":" + String(config.clutchPullAngle);
  j += ",\"rpmLow\":" + String(config.rpmLowAngle);
  j += ",\"rpmMax\":" + String(config.rpmMaxAngle);
  j += ",\"upAngle\":" + String(config.upshiftAngle, 2);
  j += ",\"downAngle\":" + String(config.downshiftAngle, 2);
  j += ",\"stepsRev\":" + String(config.stepsPerRevolution, 2);
  j += ",\"stepSpeed\":" + String(config.stepperSpeed, 2);
  j += ",\"clPullD\":" + String((unsigned long)config.clutchPullDelay);
  j += ",\"engCutD\":" + String((unsigned long)config.engineCutDelay);
  j += ",\"gearD\":" + String((unsigned long)config.gearShiftDelay);
  j += ",\"engRestoreD\":" + String((unsigned long)config.engineRestoreDelay);
  j += ",\"clReleaseD\":" + String((unsigned long)config.clutchReleaseDelay);
  j += ",\"shiftInt\":" + String((unsigned long)config.shiftInterval);
  j += ",\"simSpeed\":" + String(config.simulationSpeed, 2);
  j += ",\"maxSpeed\":" + String(config.maxSimulationSpeed, 2);
  j += "}";
  return j;
}

String jsonStatus() {
  int targetGear = automaticMode ? calculateTargetGear(simulatedSpeed) : manualTarget;
  String j = "{";
  j += "\"speed\":" + String(simulatedSpeed, 2);
  j += ",\"gear\":\"" + gearName(currentGear) + "\"";
  j += ",\"gearNumber\":" + String(currentGear);
  j += ",\"target\":" + String(targetGear);
  j += ",\"targetName\":\"" + gearName(targetGear) + "\"";
  j += ",\"state\":\"" + stateName() + "\"";
  j += ",\"action\":\"" + lastAction + "\"";
  j += ",\"engine\":" + String(engineRunning ? "true" : "false");
  j += ",\"clutch\":" + String(clutchState == CLUTCH_PULLED ? "true" : "false");
  j += ",\"rpmAngle\":" + String(rpmAngleNow);
  j += ",\"stepperBusy\":" + String(stepperBusy ? "true" : "false");
  j += ",\"automatic\":" + String(automaticMode ? "true" : "false");
  j += ",\"emergency\":" + String(emergencyStop ? "true" : "false");
  j += ",\"shiftFrom\":" + String(shiftFromGear);
  j += ",\"shiftTo\":" + String(shiftToGear);
  j += ",\"upshift\":" + String(shiftIsUpshift ? "true" : "false");
  j += ",\"angle\":" + String(currentStepperAngle, 2);
  j += "}";
  return j;
}

// ----------------------------- WEB PAGE ------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
<title>ESP32 AMT</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif;background:#f4f6f8;color:#17202a}
button,input{font:inherit}
.app{max-width:900px;margin:auto;padding:18px}
.header{display:flex;align-items:center;justify-content:space-between;margin-bottom:18px}
.logo{font-size:25px;font-weight:800}
.connection{display:flex;align-items:center;gap:7px;font-size:13px;color:#6b7280}
.dot{width:9px;height:9px;background:#22c55e;border-radius:50%}
.nav{display:grid;grid-template-columns:repeat(4,1fr);gap:7px;background:#e9edf1;padding:5px;border-radius:14px;margin-bottom:15px}
.nav button{border:0;background:transparent;padding:11px 4px;border-radius:10px;color:#69717c;font-size:12px;font-weight:700}
.nav button.active{background:#fff;color:#111827;box-shadow:0 2px 8px rgba(0,0,0,.08)}
.page{display:none}
.page.active{display:block}
.card{background:#fff;border-radius:20px;padding:18px;margin-bottom:14px;box-shadow:0 5px 25px rgba(20,30,40,.06)}
.card-title{font-size:15px;font-weight:800;margin-bottom:15px}
.speed-card{text-align:center;padding:30px 15px}
.speed-label{color:#89919b;font-size:13px;font-weight:700;text-transform:uppercase;letter-spacing:1px}
.speed{font-size:65px;line-height:1;font-weight:900;margin:10px 0 4px}
.speed-unit{color:#89919b;font-size:14px}
.gear-box{margin-top:25px;display:flex;justify-content:center;align-items:center;gap:30px}
.gear-current,.target{width:115px;height:115px;border-radius:30px;display:flex;flex-direction:column;justify-content:center}
.gear-current{background:#f2f4f6}
.target{background:#eef4ff;color:#2563eb}
.gear-number{font-size:50px;font-weight:900}
.gear-caption{font-size:11px;color:#8a929b;font-weight:800}
.target .gear-caption{color:#2563eb}
.arrow{font-size:35px;font-weight:900;color:#2563eb}
.status-big{font-size:20px;font-weight:800}
.status-sub{color:#89919b;margin-top:4px;font-size:13px}
.progress{margin-top:18px}
.step{display:flex;align-items:center;gap:10px;padding:9px 0;font-size:13px;color:#9aa1a9}
.step-circle{width:25px;height:25px;border-radius:50%;background:#e9edf1;display:flex;align-items:center;justify-content:center;font-size:11px;font-weight:800}
.step.active{color:#2563eb;font-weight:800}
.step.active .step-circle{background:#2563eb;color:#fff}
.step.done{color:#16a34a}
.step.done .step-circle{background:#dcfce7;color:#16a34a}
label{display:block;font-size:12px;font-weight:800;color:#6b7280;margin:10px 0 7px}
input[type=number],input[type=text]{width:100%;border:1px solid #e0e4e8;border-radius:11px;padding:12px;background:#fafbfc;outline:none;color:#111827}
input:focus{border-color:#2563eb;background:#fff}
.input-row{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.range{width:100%;margin-top:12px}
.speed-input{font-size:25px!important;font-weight:800;text-align:center}
button.action{width:100%;border:0;border-radius:12px;padding:13px;font-weight:800;margin-top:10px}
.primary{background:#111827;color:#fff}
.light{background:#eef1f4;color:#111827}
.danger{background:#fee2e2;color:#b91c1c}
.green{background:#dcfce7;color:#15803d}
.gear-row{display:grid;grid-template-columns:60px 1fr 110px;align-items:center;padding:13px 0;border-bottom:1px solid #edf0f2}
.gear-row:last-child{border-bottom:0}
.gear-badge{width:40px;height:40px;display:flex;align-items:center;justify-content:center;border-radius:12px;background:#f0f2f4;font-weight:900}
.actuator{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.actuator-box{padding:15px;background:#f6f7f8;border-radius:14px}
.actuator-name{color:#7b838c;font-size:12px}
.actuator-value{margin-top:5px;font-weight:900}
@media(min-width:700px){.app{padding:30px}.dashboard-grid{display:grid;grid-template-columns:1.2fr .8fr;gap:14px}}
</style>
</head>
<body>
<div class="app">

<div class="header">
  <div class="logo">ESP32 AMT</div>
  <div class="connection"><div class="dot"></div>CONNECTED</div>
</div>

<div class="nav">
  <button class="navbtn active" onclick="showPage('dashboard',this)">Dashboard</button>
  <button class="navbtn" onclick="showPage('simulator',this)">Simulator</button>
  <button class="navbtn" onclick="showPage('map',this)">Gear Map</button>
  <button class="navbtn" onclick="showPage('settings',this)">Settings</button>
</div>

<!-- DASHBOARD -->
<div id="dashboard" class="page active">
<div class="dashboard-grid">
  <div class="card speed-card">
    <div class="speed-label">Current Speed</div>
    <div class="speed" id="dashSpeed">0.0</div>
    <div class="speed-unit">KM/H</div>
    <div class="gear-box">
      <div class="gear-current"><div class="gear-number" id="currentGear">N</div><div class="gear-caption">CURRENT</div></div>
      <div class="arrow">&rarr;</div>
      <div class="target"><div class="gear-number" id="targetGear">N</div><div class="gear-caption">TARGET</div></div>
    </div>
  </div>
  <div>
    <div class="card">
      <div class="card-title">Transmission Status</div>
      <div class="status-big" id="statusText">READY</div>
      <div class="status-sub" id="actionText">System ready</div>
      <div class="progress">
        <div class="step" id="p1"><div class="step-circle">1</div>Clutch</div>
        <div class="step" id="p2"><div class="step-circle">2</div>RPM Reduce</div>
        <div class="step" id="p3"><div class="step-circle">3</div>Engine Cut</div>
        <div class="step" id="p4"><div class="step-circle">4</div>Gear Shift</div>
        <div class="step" id="p5"><div class="step-circle">5</div>Engine Restore</div>
        <div class="step" id="p6"><div class="step-circle">6</div>RPM Restore</div>
        <div class="step" id="p7"><div class="step-circle">7</div>Clutch Release</div>
      </div>
    </div>
    <div class="card">
      <div class="card-title">Actuators</div>
      <div class="actuator">
        <div class="actuator-box"><div class="actuator-name">ENGINE</div><div class="actuator-value" id="engineStatus">ON</div></div>
        <div class="actuator-box"><div class="actuator-name">CLUTCH</div><div class="actuator-value" id="clutchStatus">NORMAL</div></div>
        <div class="actuator-box"><div class="actuator-name">RPM SERVO</div><div class="actuator-value" id="rpmStatus">--</div></div>
        <div class="actuator-box"><div class="actuator-name">STEPPER</div><div class="actuator-value" id="stepperStatus">READY</div></div>
        <div class="actuator-box"><div class="actuator-name">MODE</div><div class="actuator-value" id="modeStatus">AUTO</div></div>
      </div>
    </div>
  </div>
</div>
</div>

<!-- SIMULATOR -->
<div id="simulator" class="page">
  <div class="card">
    <div class="card-title">Live Speed Simulator</div>
    <label>Simulated Speed (km/h)</label>
    <input id="speedInput" class="speed-input" type="number" min="0" step="0.1" value="0">
    <input id="speedRange" class="range" type="range" min="0" max="75" step="0.1" value="0" oninput="rangeChanged()">
    <button class="action primary" onclick="applySpeed()">APPLY SPEED</button>
    <button class="action light" onclick="setSpeed(0)">SET 0 KM/H</button>
  </div>

  <div class="card">
    <div class="card-title">Simulation Result</div>
    <div class="gear-row"><div class="gear-badge">S</div><div>Current Speed</div><strong id="simSpeed">0.0 km/h</strong></div>
    <div class="gear-row"><div class="gear-badge">G</div><div>Current Gear</div><strong id="simCurrent">N</strong></div>
    <div class="gear-row"><div class="gear-badge">T</div><div>Target Gear</div><strong id="simTarget">N</strong></div>
  </div>

  <div class="card">
    <div class="card-title">Mode</div>
    <button class="action green" onclick="setAuto(1)">AUTOMATIC MODE</button>
    <div class="card-title" style="margin-top:20px">Manual Gear Test</div>
    <button class="action light" onclick="manualGear(0)">NEUTRAL</button>
    <button class="action light" onclick="manualGear(1)">GEAR 1</button>
    <button class="action light" onclick="manualGear(2)">GEAR 2</button>
    <button class="action light" onclick="manualGear(3)">GEAR 3</button>
    <button class="action light" onclick="manualGear(4)">GEAR 4</button>
  </div>

  <div class="card">
    <button class="action danger" onclick="doEmergency()">EMERGENCY STOP</button>
    <button class="action green" onclick="doClear()">CLEAR / READY</button>
  </div>
</div>

<!-- GEAR MAP -->
<div id="map" class="page">
  <div class="card">
    <div class="card-title">Gear Speed Map</div>
    <div class="gear-row"><div class="gear-badge">1</div><div>Gear 1 up to</div><strong id="mapG1">--</strong></div>
    <div class="gear-row"><div class="gear-badge">2</div><div>Gear 2 up to</div><strong id="mapG2">--</strong></div>
    <div class="gear-row"><div class="gear-badge">3</div><div>Gear 3 up to</div><strong id="mapG3">--</strong></div>
    <div class="gear-row"><div class="gear-badge">4</div><div>Gear 4 up to</div><strong id="mapG4">--</strong></div>
  </div>
  <div class="card">
    <div class="card-title">How the Map Works</div>
    <p>0 km/h is Neutral. From there the speed ranges below each limit select the gear.</p>
    <p>Default: 0-15 Gear 1, 15-30 Gear 2, 30-50 Gear 3, above 50 Gear 4.</p>
  </div>
</div>

<!-- SETTINGS -->
<div id="settings" class="page">
  <div class="card">
    <div class="card-title">Gear Speed Settings</div>
    <div class="input-row">
      <div><label>Gear 1 Max</label><input id="g1" type="number" step="0.1"></div>
      <div><label>Gear 2 Max</label><input id="g2" type="number" step="0.1"></div>
    </div>
    <div class="input-row">
      <div><label>Gear 3 Max</label><input id="g3" type="number" step="0.1"></div>
      <div><label>Gear 4 Max</label><input id="g4" type="number" step="0.1"></div>
    </div>
    <label>Downshift Hysteresis</label>
    <input id="hyst" type="number" step="0.1">
  </div>

  <div class="card">
    <div class="card-title">Clutch</div>
    <div class="input-row">
      <div><label>Normal Angle</label><input id="clNormal" type="number"></div>
      <div><label>Pull Angle</label><input id="clPull" type="number"></div>
    </div>
  </div>

  <div class="card">
    <div class="card-title">RPM Servo</div>
    <div class="input-row">
      <div><label>Low / 0 RPM Angle (X)</label><input id="rpmLow" type="number" min="0" max="180" step="1"></div>
      <div><label>Max RPM Angle (Y)</label><input id="rpmMax" type="number" min="0" max="180" step="1"></div>
    </div>
    <div class="status-sub">Every shift: Y to X before engine cut, then X to Y after engine restore.</div>
  </div>

  <div class="card">
    <div class="card-title">Gear Actuator</div>
    <div class="input-row">
      <div><label>Upshift Angle</label><input id="upAngle" type="number" step="0.1"></div>
      <div><label>Downshift Angle</label><input id="downAngle" type="number" step="0.1"></div>
    </div>
    <label>Steps / Revolution</label>
    <input id="stepsRev" type="number" step="1">
    <label>Stepper Speed (steps/sec)</label>
    <input id="stepSpeed" type="number" step="1">
  </div>

  <div class="card">
    <div class="card-title">Shift Timing</div>
    <label>Clutch Pull Delay (ms)</label><input id="clPullD" type="number">
    <label>Engine Cut Delay / RPM settle (ms)</label><input id="engCutD" type="number">
    <label>Gear Shift Delay (ms)</label><input id="gearD" type="number">
    <label>Engine Restore Delay (ms)</label><input id="engRestoreD" type="number">
    <label>Clutch Release Delay (ms)</label><input id="clReleaseD" type="number">
    <label>Minimum Shift Interval (ms)</label><input id="shiftInt" type="number">
  </div>

  <div class="card">
    <button class="action primary" onclick="saveSettings()">SAVE SETTINGS</button>
    <button class="action danger" onclick="factoryReset()">FACTORY RESET</button>
  </div>
</div>

</div>

<script>
function $(id){return document.getElementById(id);}

function showPage(id,button){
  document.querySelectorAll('.page').forEach(function(p){p.classList.remove('active');});
  document.querySelectorAll('.navbtn').forEach(function(b){b.classList.remove('active');});
  $(id).classList.add('active');
  button.classList.add('active');
}

function rangeChanged(){ $('speedInput').value = $('speedRange').value; }

function setSpeed(v){ $('speedInput').value=v; $('speedRange').value=v; applySpeed(); }

async function applySpeed(){
  var s = $('speedInput').value;
  $('speedRange').value = s;
  try{ await fetch('/api/speed?value='+encodeURIComponent(s)); }catch(e){}
}

async function manualGear(g){
  if(!confirm('Command gear '+(g===0?'Neutral':g)+'? This switches to MANUAL mode.')) return;
  try{ await fetch('/api/manual?gear='+g); }catch(e){}
}

async function setAuto(v){ try{ await fetch('/api/auto?value='+v); }catch(e){} }
async function doEmergency(){ try{ await fetch('/api/emergency'); }catch(e){} }
async function doClear(){ try{ await fetch('/api/clear'); }catch(e){} }

var configFields = ['g1','g2','g3','g4','hyst','clNormal','clPull','rpmLow','rpmMax',
  'upAngle','downAngle','stepsRev','stepSpeed','clPullD','engCutD','gearD',
  'engRestoreD','clReleaseD','shiftInt'];

async function loadConfig(){
  try{
    var r = await fetch('/api/config');
    var d = await r.json();
    configFields.forEach(function(k){ if(d[k]!==undefined) $(k).value = d[k]; });
    $('speedInput').value = d.simSpeed;
    $('speedRange').value = d.simSpeed;
    $('speedRange').max = d.maxSpeed;
    $('mapG1').innerText = d.g1+' km/h';
    $('mapG2').innerText = d.g2+' km/h';
    $('mapG3').innerText = d.g3+' km/h';
    $('mapG4').innerText = d.g4+' km/h';
  }catch(e){ console.log(e); }
}

async function saveSettings(){
  var p = new URLSearchParams();
  configFields.forEach(function(k){ p.append(k, $(k).value); });
  try{
    var r = await fetch('/api/save',{method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:p});
    if(r.ok){ alert('Settings saved'); loadConfig(); }
    else { alert('Save refused: '+(await r.text())); }
  }catch(e){ alert('Save failed'); }
}

async function factoryReset(){
  if(!confirm('Restore all factory defaults?')) return;
  try{
    var r = await fetch('/api/reset');
    if(r.ok){ await loadConfig(); alert('Factory defaults restored'); }
    else { alert('Reset refused: '+(await r.text())); }
  }catch(e){}
}

function updateProgress(d){
  for(var i=1;i<=7;i++){ $('p'+i).classList.remove('active','done'); }
  var order = ['CLUTCH PULL','RPM REDUCE','ENGINE CUT','GEAR SHIFT','ENGINE RESTORE','RPM RESTORE','CLUTCH RELEASE'];
  var s = d.state;
  if(s==='STEPPER RETURN') s='GEAR SHIFT';
  var idx = order.indexOf(s);
  if(idx<0) return;
  for(var i=0;i<idx;i++) $('p'+(i+1)).classList.add('done');
  $('p'+(idx+1)).classList.add('active');
}

async function updateStatus(){
  try{
    var r = await fetch('/api/status');
    var d = await r.json();
    $('dashSpeed').innerText = Number(d.speed).toFixed(1);
    $('currentGear').innerText = d.gear;
    $('targetGear').innerText = d.targetName;
    $('statusText').innerText = d.state;
    $('actionText').innerText = d.action;
    $('engineStatus').innerText = d.engine ? 'ON' : 'OFF';
    $('clutchStatus').innerText = d.clutch ? 'PULLED' : 'NORMAL';
    $('rpmStatus').innerText = Number(d.rpmAngle).toFixed(0)+'\u00B0';
    $('stepperStatus').innerText = d.stepperBusy ? 'MOVING' : 'READY';
    $('modeStatus').innerText = d.emergency ? 'E-STOP' : (d.automatic ? 'AUTO' : 'MANUAL');
    $('simSpeed').innerText = Number(d.speed).toFixed(1)+' km/h';
    $('simCurrent').innerText = d.gear;
    $('simTarget').innerText = d.targetName;
    updateProgress(d);
  }catch(e){ console.log(e); }
}

loadConfig();
updateStatus();
setInterval(updateStatus, 400);
</script>
</body>
</html>
)rawliteral";

// --------------------------- HTTP HANDLERS ---------------------------------
void handleRoot()   { server.send_P(200, "text/html", INDEX_HTML); }
void handleConfig() { server.send(200, "application/json", jsonConfig()); }
void handleStatus() { server.send(200, "application/json", jsonStatus()); }

void handleSpeed() {
  if (!server.hasArg("value")) { server.send(400, "text/plain", "Missing speed"); return; }
  float speed = clampF(server.arg("value").toFloat(), 0.0f, config.maxSimulationSpeed);
  simulatedSpeed = speed;
  config.simulationSpeed = speed;      // kept in RAM only (saved with SAVE SETTINGS)
  Serial.printf("Simulation speed: %.1f km/h\n", speed);
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  if (amtState != AMT_IDLE) { server.send(409, "text/plain", "Cannot change settings during shift"); return; }

  if (server.hasArg("g1")) config.gear1Speed = server.arg("g1").toFloat();
  if (server.hasArg("g2")) config.gear2Speed = server.arg("g2").toFloat();
  if (server.hasArg("g3")) config.gear3Speed = server.arg("g3").toFloat();
  if (server.hasArg("g4")) config.gear4Speed = server.arg("g4").toFloat();
  if (server.hasArg("hyst")) config.downshiftHysteresis = server.arg("hyst").toFloat();

  if (server.hasArg("clNormal")) config.clutchNormalAngle = server.arg("clNormal").toInt();
  if (server.hasArg("clPull"))   config.clutchPullAngle   = server.arg("clPull").toInt();
  if (server.hasArg("rpmLow"))   config.rpmLowAngle       = server.arg("rpmLow").toInt();
  if (server.hasArg("rpmMax"))   config.rpmMaxAngle       = server.arg("rpmMax").toInt();

  if (server.hasArg("upAngle"))   config.upshiftAngle   = server.arg("upAngle").toFloat();
  if (server.hasArg("downAngle")) config.downshiftAngle = server.arg("downAngle").toFloat();
  if (server.hasArg("stepsRev"))  config.stepsPerRevolution = server.arg("stepsRev").toFloat();
  if (server.hasArg("stepSpeed")) config.stepperSpeed = server.arg("stepSpeed").toFloat();

  if (server.hasArg("clPullD"))     config.clutchPullDelay    = (uint32_t)server.arg("clPullD").toInt();
  if (server.hasArg("engCutD"))     config.engineCutDelay     = (uint32_t)server.arg("engCutD").toInt();
  if (server.hasArg("gearD"))       config.gearShiftDelay     = (uint32_t)server.arg("gearD").toInt();
  if (server.hasArg("engRestoreD")) config.engineRestoreDelay = (uint32_t)server.arg("engRestoreD").toInt();
  if (server.hasArg("clReleaseD"))  config.clutchReleaseDelay = (uint32_t)server.arg("clReleaseD").toInt();
  if (server.hasArg("shiftInt"))    config.shiftInterval      = (uint32_t)server.arg("shiftInt").toInt();

  sanitizeConfig();
  saveConfig();

  // Re-apply servo positions with the new angles (only if no emergency)
  if (!emergencyStop) {
    if (clutchState == CLUTCH_NORMAL) clutchServo.write(config.clutchNormalAngle);
    else clutchServo.write(config.clutchPullAngle);
    if (engineRunning) { rpmAngleNow = config.rpmMaxAngle; rpmServo.write(rpmAngleNow); }
    else               { rpmAngleNow = config.rpmLowAngle; rpmServo.write(rpmAngleNow); }
  }

  server.send(200, "text/plain", "SAVED");
}

void handleManualShift() {
  if (!server.hasArg("gear")) { server.send(400, "text/plain", "Missing gear"); return; }
  if (emergencyStop)          { server.send(409, "text/plain", "Emergency stop active"); return; }
  int gear = server.arg("gear").toInt();
  if (gear < 0 || gear > 4)   { server.send(400, "text/plain", "Invalid gear"); return; }

  automaticMode = false;
  manualTarget = gear;       // the state machine walks there one gear at a time
  lastAction = "MANUAL SHIFT";
  server.send(200, "text/plain", "MANUAL TARGET SET");
}

void handleAuto() {
  bool on = server.hasArg("value") ? (server.arg("value").toInt() != 0) : true;
  automaticMode = on;
  if (!on) manualTarget = currentGear;
  server.send(200, "text/plain", on ? "AUTO" : "MANUAL");
}

void handleEmergency() { emergencyStopSystem(); server.send(200, "text/plain", "EMERGENCY STOP"); }
void handleClear()     { clearEmergency();      server.send(200, "text/plain", "READY"); }

void handleReset() {
  if (amtState != AMT_IDLE) { server.send(409, "text/plain", "Cannot reset during shift"); return; }
  factoryReset();
  server.send(200, "text/plain", "RESET");
}

void handleNotFound() { server.send(404, "text/plain", "404 - Not Found"); }

// ------------------------------ WIFI / SERVER ------------------------------
void startWiFi() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_NAME, AP_PASSWORD);
  if (!ok) {
    Serial.println("ERROR: WiFi AP failed");
    return;
  }
  Serial.println();
  Serial.println("========================================");
  Serial.println("          ESP32 AMT CONTROLLER");
  Serial.println("========================================");
  Serial.print("WiFi Name : "); Serial.println(AP_NAME);
  Serial.print("Password  : "); Serial.println(AP_PASSWORD);
  Serial.print("IP Address: http://"); Serial.println(WiFi.softAPIP());
  Serial.println("========================================");
}

void startWebServer() {
  server.on("/",             HTTP_GET,  handleRoot);
  server.on("/api/config",   HTTP_GET,  handleConfig);
  server.on("/api/status",   HTTP_GET,  handleStatus);
  server.on("/api/speed",    HTTP_GET,  handleSpeed);
  server.on("/api/save",     HTTP_POST, handleSave);
  server.on("/api/manual",   HTTP_GET,  handleManualShift);
  server.on("/api/auto",     HTTP_GET,  handleAuto);
  server.on("/api/emergency",HTTP_GET,  handleEmergency);
  server.on("/api/clear",    HTTP_GET,  handleClear);
  server.on("/api/reset",    HTTP_GET,  handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started.");
}

// ------------------------------- SETUP -------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Starting ESP32 AMT...");

  // GPIO first, in safe states
  pinMode(ENGINE_RELAY_PIN, OUTPUT);
  pinMode(STEPPER_STEP_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);
  pinMode(STEPPER_ENABLE_PIN, OUTPUT);
  digitalWrite(STEPPER_STEP_PIN, LOW);
  digitalWrite(STEPPER_DIR_PIN, LOW);
  stepperDisable();
  engineOn();

  loadConfig();
  simulatedSpeed = config.simulationSpeed;

  // Wi-Fi first (same order as the sketch that worked), servos afterwards
  startWiFi();
  startWebServer();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  clutchServo.setPeriodHertz(50);
  clutchServo.attach(CLUTCH_SERVO_PIN, 500, 2400);
  delay(200);                       // stagger the servos to soften the current spike
  rpmServo.setPeriodHertz(50);
  rpmServo.attach(RPM_SERVO_PIN, 500, 2400);
  delay(200);

  clutchNormal();
  delay(200);
  rpmMax();

  currentGear = 0;
  requestedGear = 0;
  manualTarget = 0;
  shiftFromGear = 0;
  shiftToGear = 0;
  amtState = AMT_IDLE;
  emergencyStop = false;
  automaticMode = true;
  lastAction = "READY";
  lastShiftCompletedAt = millis();

  Serial.println("ESP32 AMT READY. Starting gear: NEUTRAL");
}

// -------------------------------- LOOP -------------------------------------
void loop() {
  server.handleClient();
  updateStepper();
  updateAMT();

  if (stepperBusy) yield();   // keep step timing tight while moving
  else delay(1);              // idle: be nice to the Wi-Fi stack
}
