const db = require('../database/db');

class ExportController {
    async exportShiftsCsv(req, res) {
        try {
            const rows = await db.all('SELECT * FROM shift_events ORDER BY id DESC');
            res.setHeader('Content-Type', 'text/csv');
            res.setHeader('Content-Disposition', 'attachment; filename="amt_shift_events.csv"');

            let csv = 'ID,Timestamp,FromGear,ToGear,DurationMs,Success,RPM,Mode\n';
            rows.forEach(r => {
                csv += `${r.id},"${r.timestamp}",${r.from_gear},${r.to_gear},${r.duration_ms},${r.success},${r.rpm},"${r.mode}"\n`;
            });
            res.send(csv);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async exportTelemetryCsv(req, res) {
        try {
            const rows = await db.all('SELECT * FROM rpm_readings ORDER BY id DESC LIMIT 5000');
            res.setHeader('Content-Type', 'text/csv');
            res.setHeader('Content-Disposition', 'attachment; filename="amt_rpm_telemetry.csv"');

            let csv = 'ID,Timestamp,InstantaneousRPM,AverageRPM,MaxRPM,PulseCount\n';
            rows.forEach(r => {
                csv += `${r.id},"${r.timestamp}",${r.instantaneous_rpm},${r.average_rpm},${r.max_rpm},${r.pulse_count}\n`;
            });
            res.send(csv);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async exportFaultsCsv(req, res) {
        try {
            const rows = await db.all('SELECT * FROM fault_logs ORDER BY id DESC');
            res.setHeader('Content-Type', 'text/csv');
            res.setHeader('Content-Disposition', 'attachment; filename="amt_fault_logs.csv"');

            let csv = 'ID,Timestamp,FaultCode,Name,Description,Acknowledged\n';
            rows.forEach(r => {
                csv += `${r.id},"${r.timestamp}",${r.fault_code},"${r.name}","${r.description.replace(/"/g, '""')}",${r.acknowledged}\n`;
            });
            res.send(csv);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }
}

module.exports = new ExportController();

