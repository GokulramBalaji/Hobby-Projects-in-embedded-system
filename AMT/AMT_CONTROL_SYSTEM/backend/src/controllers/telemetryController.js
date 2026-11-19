const mqttHandler = require('../mqtt/mqttHandler');
const db = require('../database/db');

class TelemetryController {
    async getLive(req, res) {
        res.json(mqttHandler.getLatestTelemetry());
    }

    async getHistory(req, res) {
        const limit = parseInt(req.query.limit, 10) || 100;
        try {
            const rows = await db.all(
                `SELECT timestamp, instantaneous_rpm, average_rpm, max_rpm 
                 FROM rpm_readings 
                 ORDER BY id DESC LIMIT ?`,
                [limit]
            );
            res.json(rows.reverse());
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async getDevices(req, res) {
        try {
            const devices = await db.all('SELECT * FROM devices ORDER BY last_seen DESC');
            res.json(devices);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async getFaults(req, res) {
        try {
            const faults = await db.all('SELECT * FROM fault_logs ORDER BY id DESC LIMIT 50');
            res.json(faults);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async clearFaults(req, res) {
        try {
            await db.run('UPDATE fault_logs SET acknowledged = 1');
            res.json({ status: 'ok', message: 'All faults marked acknowledged' });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }
}

module.exports = new TelemetryController();

