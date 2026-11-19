# AMT Control System — Diagnostic & Troubleshooting Guide

## 1. Quick Diagnostic Decision Tree

```
[Issue Encountered]
 │
 ├── ESP32 Brownout / Reset Loop ───► Check Servo Power Isolation (Section 2.1)
 ├── Inaccurate / Zero RPM ──────────► Check V183 Voltage & Trimpot (Section 2.2)
 ├── Stepper Motor Buzzes / Jams ────► Check L298N Sequence & Current (Section 2.3)
 ├── Wi-Fi / MQTT Offline ───────────► Check Fallback AP & Port 1883 (Section 2.4)
 └── E-Stop Latched Permanently ─────► Check NC Switch Loop & Pin 35 (Section 2.5)
```

---

## 2. Root Cause Analysis & Resolutions

### 2.1. ESP32 Reboots or Brownout Detected During Gear Shift
- **Symptom:** The ESP32 drops Wi-Fi or reboots (`Brownout detector was triggered`) as soon as the clutch servo begins moving.
- **Root Cause:** A 25 kg·cm servo draws between 1.8A and 2.8A peak during sudden acceleration. Powering it from the ESP32 USB 5V or VIN causes the 3.3V rail to collapse below 2.8V.
- **Solution:**
  1. Power the PCA9685 `V+` terminal block from an independent **6V 5A+ DC switching power supply**.
  2. Connect the PCA9685 `VCC` logic pin to ESP32 **3.3V**.
  3. Ensure a common ground wire connects the 6V power supply negative terminal to ESP32 GND.

---

### 2.2. V183 Optical IR Sensor Shows 0 RPM or Erratic Spikes
- **Symptom:** Engine is rotating, but Cockpit reads 0 RPM, or jumps to 12,000 RPM.
- **Root Cause:**
  1. If 0 RPM: The LM393 comparator trimpot on the sensor module is set outside the optical switching threshold, or the slotted flag is misaligned.
  2. If erratic spikes: Ambient optical reflections or electrical noise causing false interrupts.
- **Solution:**
  1. Turn the onboard blue potentiometer on the V183 sensor until the indicator LED toggles cleanly when an obstacle passes through the optical slot.
  2. Verify that the signal on GPIO 34 does not exceed 3.3V (use a 1kΩ / 2kΩ resistive voltage divider if powering sensor from 5V).
  3. In **Tab 2: RPM Monitor**, increase the **Glitch Filter Window** from 200 µs to 350 µs.

---

### 2.3. Stepper Motor Vibrates, Hums, or Loses Steps
- **Symptom:** Motor shafts vibrates back and forth without continuous rotation.
- **Root Cause:**
  1. Phase pairing mismatch: Coil A and Coil B leads are inverted or cross-connected on L298N terminals.
  2. Current starvation or L298N overheating.
- **Solution:**
  1. Disconnect power and measure motor coil continuity using a multimeter. Ensure Coil A leads connect to `OUT1` and `OUT2`, and Coil B leads connect to `OUT3` and `OUT4`.
  2. In **Tab 5: Stepper Control**, reduce the maximum speed step rate from 800 steps/sec to 400 steps/sec.
  3. Check the L298N heatsink temperature. If scorching hot, upgrade to an active constant-current chopper driver (TMC2209 or A4988).

---

### 2.4. MQTT Disconnected / PC Web App Offline
- **Symptom:** Dashboard displays `OFFLINE` badge.
- **Root Cause:**
  1. Windows Defender Firewall blocking inbound connections on TCP port 1883 or 5000.
  2. ESP32 Wi-Fi station mode failed to connect to home router.
- **Solution:**
  1. If the ESP32 fails to connect to your configured router within 12 seconds, it automatically broadcasts its own Wi-Fi Access Point:
     - **SSID:** `AMT-CONTROLLER-AP`
     - **Password:** `amtadminpassword`
     - **IP:** `192.168.4.1`
  2. Connect your PC or smartphone directly to `AMT-CONTROLLER-AP` and browse to `http://192.168.4.1` to access the onboard emergency control interface!
  3. Ensure Windows Firewall allows `node.exe` on private networks.

---

### 2.5. Permanent Emergency Stop Latch
- **Symptom:** UI states `EMERGENCY STOP ACTIVE` and will not clear.
- **Root Cause:** The hardware E-Stop input is designed with a **Normally Closed (NC)** failsafe loop. If the switch wire is loose, broken, or the button is depressed, GPIO 35 reads HIGH and forces a shutdown.
- **Solution:**
  1. Ensure the emergency stop mushroom button is twisted clockwise to pop out into the released state.
  2. Check continuity across the switch contacts into GPIO 35 and GND.
  3. If testing on a workbench without a physical button wired, temporarily connect a jumper wire from **GPIO 35 to GND**.

