# AMT Hardware Electrical Wiring & Interface Engineering Specification

## 1. System Pin Mapping (ESP32-S 38-Pin Module)

| ESP32 Pin | Function | Direction | Connected Device | Electrical Logic Level | Notes / Protection |
|---|---|---|---|---|---|
| **GPIO 34** | RPM Sensor Pulse | Input | V183 Optical IR Sensor (DO) | 3.3V Logic (Voltage Divided) | Input-only pin. Requires 1kΩ/2kΩ divider if sensor is powered from 5V. |
| **GPIO 35** | Hardware E-Stop | Input | Physical Emergency Stop Switch | 3.3V Logic | Normally Closed (NC) to GND. 10kΩ pull-up to 3.3V. Wire cut trips E-Stop. |
| **GPIO 21** | I2C SDA | Bidirectional | PCA9685 Servo Driver (SDA) | 3.3V I2C | Standard ESP32 hardware I2C data line. |
| **GPIO 22** | I2C SCL | Output | PCA9685 Servo Driver (SCL) | 3.3V I2C | Standard ESP32 hardware I2C clock line (400 kHz Fast-Mode). |
| **GPIO 25** | Stepper Coil IN1 | Output | L298N Driver IN1 (or STEP) | 3.3V Logic | Coil A+ phase control (or STEP in Step/Dir mode). |
| **GPIO 26** | Stepper Coil IN2 | Output | L298N Driver IN2 (or DIR) | 3.3V Logic | Coil A- phase control (or DIR in Step/Dir mode). |
| **GPIO 27** | Stepper Coil IN3 | Output | L298N Driver IN3 (or ENA) | 3.3V Logic | Coil B+ phase control (or ENABLE in Step/Dir mode). |
| **GPIO 14** | Stepper Coil IN4 | Output | L298N Driver IN4 | 3.3V Logic | Coil B- phase control. |
| **GPIO 19** | Engine Torque Cut | Output | Optocoupled Relay Module (IN) | 3.3V Logic | Active LOW/HIGH configurable. Ignition or fuel injector cutoff. |
| **GPIO 2** | Status LED | Output | Onboard Blue LED | 3.3V Logic | High-speed pulse on E-stop; solid ON in nominal operation. |
| **GND** | System Ground | Common | Common Ground Bus Bar | 0V Reference | Star-point grounding shared between ESP32, PCA9685, L298N, and PSUs. |

---

## 2. Sensor Interfacing & Voltage Safety: V183 IR Sensor

The V183 IR optical slotted / reflective sensor contains an infrared emitter diode and a phototransistor with an LM393 voltage comparator:
- **Operating Voltage:** 3.3V to 5.0V DC.
- **Output Logic:** Active LOW pulse when slotted flag interrupts beam.
- **Voltage Safety Rule:**
  - If powered from 5V: The digital output pin (`DO`) outputs 5.0V pulses. Connecting this directly to ESP32 GPIO 34 will cause internal substrate diode conduction and permanently burn out the GPIO pad!
  - **Interfacing Solution A (Resistive Voltage Divider):** Connect sensor `DO` through a $1.0\text{ k}\Omega$ series resistor into GPIO 34, and a $2.0\text{ k}\Omega$ resistor from GPIO 34 to GND.
    $$V_{\text{ESP32}} = 5.0\text{ V} \times \frac{2.0\text{ k}\Omega}{1.0\text{ k}\Omega + 2.0\text{ k}\Omega} = 3.33\text{ V}$$
  - **Interfacing Solution B (Pure 3.3V rail):** Power the V183 module directly from the ESP32 3.3V pin (sensor current draw is only ~15 mA).

---

## 3. High-Torque Servo & PCA9685 Architecture

### Actuator: 25 kg·cm Digital Metal-Gear Servo
- **Operating Voltage:** 4.8V – 7.2V (Optimally 6.0V DC).
- **Stall Current:** Up to 2.8A peak during sudden clutch engagement/disengagement strokes.
- **Holding Current:** ~800 mA.

### Power Isolation Requirements:
> [!CAUTION]
> **NEVER POWER THE SERVO FROM THE ESP32!**
> Powering a 25 kg·cm servo from the ESP32 5V (VIN) or 3.3V pins will cause immediate supply brownout resets, permanent degradation of the onboard AMS1117 LDO, and intermittent Wi-Fi drops.

- **PCA9685 Logic Supply (`VCC`):** Connect to ESP32 **3.3V** rail so that the I2C bus operates natively at 3.3V without needing external logic level shifters.
- **PCA9685 Servo Supply (`V+`):** Connect to an independent **6.0V DC 5A–10A Switching Power Supply**.
- **Common Ground:** Connect the negative terminal of the 6V servo power supply to the ESP32 GND rail.

---

## 4. Stepper Motor Driver Analysis: NEMA 17 (423HT40-1204A) & L298N

### Motor Electrical Characteristics:
- **Rated Phase Current:** 1.2 A / phase.
- **Phase Resistance:** ~2.4 Ω.
- **Holding Torque:** 40 N·cm (0.4 N·m).
- **Inductance:** ~4.0 mH.

### Engineering Analysis of the L298N Dual H-Bridge:
The L298N utilizes bipolar junction transistors (BJT). At 1.2A coil current:
1. **Saturation Voltage Drop:** The internal transistor Darlington pair exhibits a voltage drop $V_{\text{cesat}} \approx 1.8\text{V} - 2.8\text{V}$.
2. **Thermal Dissipation:** Power lost as pure heat inside the driver:
   $$P_{\text{diss}} \approx 2 \times 1.2\text{ A} \times 2.2\text{ V} \approx 5.28\text{ Watts}$$
   Without an oversized heatsink and active forced-air cooling, the L298N will enter thermal shutdown within minutes.
3. **No Active Current Chopping:** Unlike modern constant-current chopper drivers, the L298N does not regulate coil current. Applying a 12V motor supply across a 2.4Ω winding would draw:
   $$I = \frac{12\text{V} - 2\text{V}}{2.4\Omega} \approx 4.16\text{ Amperes}$$
   This will incinerate both the NEMA 17 coils and the L298N!

### Firmware Protections Implemented:
- The firmware de-energizes all four coils (`IN1..IN4 = LOW`) whenever the motor is idle for > 400ms.
- Half-stepping 8-phase sequencing reduces mechanical vibration and acoustic resonance.

### Recommended Production Alternative:
Replace the L298N with a constant-current chopper driver:
- **TMC2209 SilentStepStick:** Delivers up to 2.0A RMS with StealthChop2 silent operation, automatic current reduction at standstill, and built-in StallGuard4 sensorless homing.
- **A4988 / DRV8825:** Standard industrial chopper drivers with on-board VREF current trimpot adjustment.
- The firmware supports both L298N 4-wire mode and STEP/DIR mode natively via configuration!

---

## 5. Engine Cut-off Relay Interfacing

- **Module:** 5V / 3.3V Optocoupled Relay Module with transistor driver and flyback protection diode.
- **Connection:**
  - `VCC`: 5V (from system 5V bus)
  - `IN`: ESP32 GPIO 19
  - `GND`: Common GND
  - `COM` & `NC/NO`: Wired into vehicle ignition coil primary circuit or ECU torque-cut input.
- **Failsafe Design:** Active level is software-configurable (default Active LOW).

---

## 6. Hardware Emergency Stop (E-Stop)

- **Switch Type:** Heavy-duty mushroom push-button, latching twist-to-release.
- **Contact Wiring:** **Normally Closed (NC)** contact loop between GPIO 35 and GND, with an external $10\text{ k}\Omega$ pull-up resistor to 3.3V.
- **Failsafe Operation:**
  - Normal state: Closed circuit to GND (GPIO 35 reads LOW).
  - Button pressed or cable broken/disconnected: Circuit opens, pull-up pulls GPIO 35 to HIGH, immediately triggering the interrupt failsafe shutdown.

