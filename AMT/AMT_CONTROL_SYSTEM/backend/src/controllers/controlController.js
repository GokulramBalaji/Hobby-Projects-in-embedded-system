const mqttHandler = require('../mqtt/mqttHandler');
const hardwareSimulator = require('../simulation/hardwareSimulator');

class ControlController {
    async setGear(req, res) {
        const { gear, action } = req.body;

        if (action === 'UP') {
            if (hardwareSimulator.running) {
                const ok = hardwareSimulator.requestShift(hardwareSimulator.currentGear + 1);
                return res.json({ status: ok ? 'ok' : 'rejected' });
            }
            const ok = mqttHandler.sendCommand('UPSHIFT');
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        if (action === 'DOWN') {
            if (hardwareSimulator.running) {
                const ok = hardwareSimulator.requestShift(hardwareSimulator.currentGear - 1);
                return res.json({ status: ok ? 'ok' : 'rejected' });
            }
            const ok = mqttHandler.sendCommand('DOWNSHIFT');
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        if (gear !== undefined) {
            const target = parseInt(gear, 10);
            if (hardwareSimulator.running) {
                const ok = hardwareSimulator.requestShift(target);
                return res.json({ status: ok ? 'ok' : 'rejected' });
            }
            const ok = mqttHandler.sendCommand('SHIFT', { gear: target });
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        res.status(400).json({ error: 'Invalid gear parameter' });
    }

    async setMode(req, res) {
        const { mode } = req.body; // 'AUTO' or 'MANUAL'
        if (hardwareSimulator.running) {
            hardwareSimulator.setMode(mode === 'AUTO' || mode === 1 ? 'AUTOMATIC' : 'MANUAL');
            return res.json({ status: 'ok', mode });
        }
        const ok = mqttHandler.sendCommand('SET_MODE', { mode });
        res.json({ status: ok ? 'ok' : 'offline' });
    }

    async setClutch(req, res) {
        const { disengage } = req.body;
        if (hardwareSimulator.running) {
            hardwareSimulator.clutchAngle = disengage ? 95 : 15;
            return res.json({ status: 'ok' });
        }
        const ok = mqttHandler.sendCommand('CLUTCH', { disengage: !!disengage });
        res.json({ status: ok ? 'ok' : 'offline' });
    }

    async controlStepper(req, res) {
        const { action, steps, gear, position } = req.body;

        if (action === 'JOG') {
            if (hardwareSimulator.running) {
                hardwareSimulator.stepperPosition += (steps || 0);
                return res.json({ status: 'ok', position: hardwareSimulator.stepperPosition });
            }
            const ok = mqttHandler.sendCommand('STEPPER_JOG', { steps: parseInt(steps, 10) });
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        if (action === 'HOME') {
            if (hardwareSimulator.running) {
                hardwareSimulator.stepperPosition = 0;
                return res.json({ status: 'ok', position: 0 });
            }
            const ok = mqttHandler.sendCommand('STEPPER_HOME');
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        if (action === 'SET_ZERO') {
            if (hardwareSimulator.running) {
                hardwareSimulator.stepperPosition = 0;
                return res.json({ status: 'ok' });
            }
            const ok = mqttHandler.sendCommand('STEPPER_SET_ZERO');
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        if (action === 'CALIBRATE_GEAR') {
            const ok = mqttHandler.sendCommand('CALIBRATE_GEAR', { gear, position });
            return res.json({ status: ok ? 'ok' : 'offline' });
        }

        res.status(400).json({ error: 'Unknown stepper action' });
    }

    async emergencyStop(req, res) {
        const { active } = req.body;
        const isEstop = (active !== false);

        if (hardwareSimulator.running) {
            hardwareSimulator.setEmergency(isEstop);
        }
        mqttHandler.sendEmergency(isEstop, isEstop ? 'E-Stop triggered via PC Web App' : 'E-Stop cleared via PC Web App');
        res.json({ status: 'ok', emergency: isEstop });
    }

    async resetSystem(req, res) {
        if (hardwareSimulator.running) {
            hardwareSimulator.currentGear = 0;
            hardwareSimulator.engineRpm = 950;
            hardwareSimulator.speedKmh = 0;
            hardwareSimulator.isShifting = false;
            hardwareSimulator.setEmergency(false);
            return res.json({ status: 'ok', message: 'Simulator reset to neutral' });
        }
        mqttHandler.sendCommand('RESET');
        res.json({ status: 'ok', message: 'Reboot command dispatched to ESP32' });
    }

    async toggleSimulation(req, res) {
        const { enabled } = req.body;
        if (enabled) {
            hardwareSimulator.start();
        } else {
            hardwareSimulator.stop();
        }
        res.json({ status: 'ok', simulation: hardwareSimulator.running });
    }
}

module.exports = new ControlController();

