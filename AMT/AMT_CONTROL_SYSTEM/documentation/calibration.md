# AMT Actuator & Sensor Calibration Engineering Guide

## 1. V183 IR Optical Tachometer Calibration

The V183 optical sensor detects encoder flags attached to the engine flywheel or shaft.

### Physical Alignment:
1. Ensure the reflective/slotted optical disc flags pass cleanly through the sensor optical axis with 1.5mm to 3.0mm clearance.
2. Ensure no stray ambient sunlight or high-frequency fluorescent lighting strikes the phototransistor directly.

### Software Calibration:
1. Open the PC Cockpit and navigate to **Tab 2: RPM Monitor**.
2. **Set PPR (Pulses Per Revolution):**
   - Single flag disc: `PPR = 1`
   - Dual flag disc: `PPR = 2`
   - 4-tooth disc: `PPR = 4`
3. **Glitch Filter Window:**
   - Default: `200 µs`.
   - If erratic high-RPM spikes occur due to edge chatter, increase filter to `300 µs` - `500 µs`.
4. **Calibration Factor:** Compare instantaneous RPM against a stroboscopic optical tachometer and fine-tune the calibration factor (e.g. `1.02`).

---

## 2. Clutch Servo Angle & Throw Calibration

The 25 kg·cm digital servo controls clutch plate disengagement via a mechanical linkage.

### Procedure:
1. Navigate to **Tab 4: Servo Control**.
2. **Calibrate Engaged Angle (Resting):**
   - Adjust the slider until the clutch release bearing has zero preload and the clutch spring fully clamps the friction plate.
   - Typically: **15° to 20°**.
3. **Calibrate Disengaged Angle (Pulled):**
   - Click "DISENGAGE CLUTCH" and adjust angle until the clutch friction disc spins completely freely with zero drag.
   - Avoid over-travel which could stall the servo against physical stops.
   - Typically: **90° to 105°**.
4. Click **"Save Calibration to ESP32 Flash"**.

---

## 3. Stepper Motor Gear Gate Position Presets

The NEMA 17 stepper moves the gear selector drum or shift fork.

### Procedure:
1. Navigate to **Tab 5: Stepper Control**.
2. Command the motor to **Neutral (Home / 0 Steps)**.
3. Use the Jog buttons (`-100`, `-10`, `+10`, `+100`) to physically engage **Gear 1**.
   - Note the step count (e.g. `500 steps`).
4. Repeat jogging for **Gear 2** (e.g. `1000 steps`), **Gear 3** (e.g. `1500 steps`), and **Gear 4** (e.g. `2000 steps`).
5. Enter these values into the Gear Preset table and click **"Save Step Presets to ESP32 Flash"**.

---

## 4. Engine Torque-Cut Timing Calibration

During shifts, cutting ignition/fuel unloads dog clutch teeth, permitting smooth gear engagement without clutch wear:
1. Navigate to **Tab 6: Auto Mode**.
2. Set **Ignition Cut Duration**:
   - Too short (< 40ms): Dog teeth remain loaded under engine torque, causing stepper stall or grinding.
   - Too long (> 200ms): Vehicle experiences excessive deceleration and shift lag.
   - Optimal range: **70 ms – 110 ms**.

