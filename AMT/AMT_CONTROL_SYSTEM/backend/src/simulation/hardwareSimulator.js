const mqttHandler = require('../mqtt/mqttHandler');
const topics = require('../mqtt/topics');
const db = require('../database/db');

class HardwareSimulator {
    constructor() {
        this.running = false;
        this.timer = null;
        this.currentGear = 0; // Neutral start
        this.targetGear = 0;
        this.isShifting = false;
        this.shiftState = 0; // 0: Idle, 1: Clutch disengage, 2: Engine cut, 3: Stepper move, 4: Engine restore, 5: Clutch engage
        this.shiftStartTime = 0;
        this.mode = 'AUTOMATIC'; // 'MANUAL' or 'AUTOMATIC'
        this.engineRpm = 950;
        this.throttle = 0.65;
        this.speedKmh = 0;
        this.clutchAngle = 15;
        this.stepperPosition = 0;
        this.engineCut = false;
        this.emergency = false;
        this.totalShifts = 0;
        this.gearStepMap = [0, 500, 1000, 1500, 2000];
        this.gearRatios = [0, 3.8, 2.2, 1.4, 1.0];
    }

    start() {
        if (this.running) return;
        this.running = true;
        console.log('[SIMULATOR] Hardware Simulator started (Generating realistic AMT kinetics).');
        this.timer = setInterval(() => this.tick(), 100); // 10Hz simulation clock
    }

    stop() {
        this.running = false;
        if (this.timer) {
            clearInterval(this.timer);
            this.timer = null;
        }
        console.log('[SIMULATOR] Hardware Simulator stopped.');
    }

    setMode(newMode) {
        this.mode = newMode;
        console.log(`[SIMULATOR] Mode switched to: ${newMode}`);
    }

    setEmergency(active) {
        this.emergency = active;
        if (active) {
            this.engineCut = true;
            this.clutchAngle = 95;
            this.isShifting = false;
            console.warn('[SIMULATOR] Emergency Stop triggered in simulator.');
        } else {
            this.engineCut = false;
            this.clutchAngle = 15;
            console.log('[SIMULATOR] Emergency Stop cleared in simulator.');
        }
    }

    requestShift(target) {
        if (this.isShifting || this.emergency) return false;
        if (target === this.currentGear || target > 4 || target < 0) return false;

        this.targetGear = target;
        this.isShifting = true;
        this.shiftState = 1;
        this.shiftStartTime = Date.now();
        console.log(`[SIMULATOR] Initiating shift sequence: Gear ${this.currentGear} -> ${this.targetGear}`);
        return true;
    }

    tick() {
        if (this.emergency) {
            this.engineRpm = Math.max(0, this.engineRpm - 200);
            this.speedKmh = Math.max(0, this.speedKmh - 1.5);
            this.publishTelemetry();
            return;
        }

        const now = Date.now();

        // 1. Shifting State Machine Simulation
        if (this.isShifting) {
            const elapsed = now - this.shiftStartTime;

            if (this.shiftState === 1) { // Disengaging Clutch (0-200ms)
                this.clutchAngle = Math.min(95, 15 + (elapsed / 200) * 80);
                if (elapsed >= 200) {
                    this.shiftState = 2; // Engine torque cut
                    this.engineCut = true;
                }
            } else if (this.shiftState === 2) { // Torque cut delay (200-280ms)
                if (elapsed >= 280) {
                    this.shiftState = 3; // Stepper motor physical move
                }
            } else if (this.shiftState === 3) { // Stepper Moving (280-600ms)
                const moveElapsed = elapsed - 280;
                const startPos = this.gearStepMap[this.currentGear];
                const endPos = this.gearStepMap[this.targetGear];
                const progress = Math.min(1.0, moveElapsed / 320);
                this.stepperPosition = Math.round(startPos + (endPos - startPos) * progress);

                if (moveElapsed >= 320) {
                    this.shiftState = 4; // Restore engine torque
                    this.engineCut = false;
                }
            } else if (this.shiftState === 4) { // Restore delay (600-660ms)
                if (elapsed >= 660) {
                    this.shiftState = 5; // Re-engaging clutch
                }
            } else if (this.shiftState === 5) { // Re-engaging clutch (660-880ms)
                const clutchElapsed = elapsed - 660;
                this.clutchAngle = Math.max(15, 95 - (clutchElapsed / 220) * 80);

                if (clutchElapsed >= 220) {
                    // Shift Complete
                    const durationMs = elapsed;
                    const fromG = this.currentGear;
                    this.currentGear = this.targetGear;
                    this.isShifting = false;
                    this.shiftState = 0;
                    this.totalShifts++;

                    // Adjust engine RPM post-shift based on new gear ratio
                    if (this.currentGear > 0 && fromG > 0) {
                        this.engineRpm = this.engineRpm * (this.gearRatios[this.currentGear] / this.gearRatios[fromG]);
                    }

                    // Emit shift event through MQTT handler
                    mqttHandler.handleIncomingMessage(topics.EVENTS, Buffer.from(JSON.stringify({
                        event: 'SHIFT_EVENT',
                        from_gear: fromG,
                        to_gear: this.currentGear,
                        duration_ms: durationMs,
                        success: true,
                        rpm: Math.round(this.engineRpm),
                        timestamp: now
                    })));

                    console.log(`[SIMULATOR] Shift complete: Gear ${this.currentGear} in ${durationMs}ms.`);
                }
            }
        } else {
            // 2. Normal Vehicle Dynamics & Engine Kinetics
            if (this.currentGear === 0) {
                // Neutral idle
                this.engineRpm = 950 + (Math.sin(now / 400) * 40);
                this.speedKmh = Math.max(0, this.speedKmh - 0.2);
            } else {
                // In gear: RPM rises with throttle
                const accelRate = 50 * this.throttle;
                this.engineRpm += accelRate;

                // Speed proportional to RPM and gear ratio
                this.speedKmh = (this.engineRpm / 1000) * (26 / this.gearRatios[this.currentGear]);

                // 3. Automatic Mode Shifting Decision
                if (this.mode === 'AUTOMATIC') {
                    if (this.engineRpm >= 3400 && this.currentGear < 4) {
                        this.requestShift(this.currentGear + 1);
                    } else if (this.engineRpm <= 1400 && this.currentGear > 1) {
                        this.requestShift(this.currentGear - 1);
                    }
                }

                // If 4th gear maxes out, oscillate realistic top speed
                if (this.engineRpm > 4200 && this.currentGear === 4) {
                    this.engineRpm = 4200 + (Math.sin(now / 600) * 50);
                }
            }
        }

        this.publishTelemetry();
    }

    publishTelemetry() {
        const payload = {
            rpm: Math.round(this.engineRpm),
            avg_rpm: Math.round(this.engineRpm),
            max_rpm: 4850,
            gear: this.currentGear,
            target_gear: this.targetGear,
            mode: this.mode,
            clutch_angle: Math.round(this.clutchAngle),
            clutch_disengaged: this.clutchAngle > 50,
            stepper_position: this.stepperPosition,
            stepper_moving: this.isShifting && this.shiftState === 3,
            engine_cut: this.engineCut,
            emergency: this.emergency,
            speed_kmh: Math.round(this.speedKmh * 10) / 10,
            total_shifts: this.totalShifts,
            timestamp: Date.now(),
            device_online: true
        };

        mqttHandler.handleIncomingMessage(topics.TELEMETRY, Buffer.from(JSON.stringify(payload)));
    }
}

module.exports = new HardwareSimulator();

