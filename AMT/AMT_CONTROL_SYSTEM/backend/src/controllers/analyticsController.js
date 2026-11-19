const db = require('../database/db');

class AnalyticsController {
    async getSummary(req, res) {
        try {
            const shiftStats = await db.get(`
                SELECT 
                    COUNT(*) as total_shifts,
                    SUM(CASE WHEN success = 1 THEN 1 ELSE 0 END) as successful_shifts,
                    SUM(CASE WHEN success = 0 THEN 1 ELSE 0 END) as failed_shifts,
                    AVG(duration_ms) as avg_duration_ms,
                    MIN(duration_ms) as min_duration_ms,
                    MAX(duration_ms) as max_duration_ms
                FROM shift_events
            `);

            const rpmStats = await db.get(`
                SELECT 
                    MAX(max_rpm) as max_recorded_rpm,
                    AVG(average_rpm) as avg_engine_rpm
                FROM rpm_readings
            `);

            const total = shiftStats.total_shifts || 0;
            const success = shiftStats.successful_shifts || 0;
            const successRate = total > 0 ? (success / total) * 100 : 100;
            const avgTime = shiftStats.avg_duration_ms || 320;

            // Automotive performance score algorithm:
            // Penalize high shift duration (> 500ms) and low success rate
            let score = 100;
            if (successRate < 95) score -= (95 - successRate) * 2;
            if (avgTime > 350) score -= Math.min(25, (avgTime - 350) / 10);
            score = Math.max(10, Math.min(100, Math.round(score)));

            res.json({
                total_shifts: total,
                successful_shifts: success,
                failed_shifts: shiftStats.failed_shifts || 0,
                success_rate: Math.round(successRate * 10) / 10,
                avg_duration_ms: Math.round(avgTime),
                min_duration_ms: shiftStats.min_duration_ms || 0,
                max_duration_ms: shiftStats.max_duration_ms || 0,
                max_recorded_rpm: Math.round(rpmStats.max_recorded_rpm || 0),
                avg_engine_rpm: Math.round(rpmStats.avg_engine_rpm || 0),
                performance_score: score
            });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async getGearUtilization(req, res) {
        try {
            const rows = await db.all(`
                SELECT to_gear as gear, COUNT(*) as count 
                FROM shift_events 
                GROUP BY to_gear 
                ORDER BY to_gear
            `);

            const utilization = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0 };
            rows.forEach(r => { utilization[r.gear] = r.count; });

            res.json(utilization);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async getRpmDistribution(req, res) {
        try {
            const rows = await db.all(`
                SELECT 
                    SUM(CASE WHEN average_rpm < 1500 THEN 1 ELSE 0 END) as idle_range,
                    SUM(CASE WHEN average_rpm >= 1500 AND average_rpm < 2500 THEN 1 ELSE 0 END) as eco_range,
                    SUM(CASE WHEN average_rpm >= 2500 AND average_rpm < 3500 THEN 1 ELSE 0 END) as cruise_range,
                    SUM(CASE WHEN average_rpm >= 3500 AND average_rpm < 4500 THEN 1 ELSE 0 END) as power_range,
                    SUM(CASE WHEN average_rpm >= 4500 THEN 1 ELSE 0 END) as high_range
                FROM rpm_readings
            `);

            res.json(rows[0] || {
                idle_range: 0,
                eco_range: 0,
                cruise_range: 0,
                power_range: 0,
                high_range: 0
            });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async getShiftLogs(req, res) {
        const limit = parseInt(req.query.limit, 10) || 50;
        try {
            const shifts = await db.all(
                `SELECT * FROM shift_events ORDER BY id DESC LIMIT ?`,
                [limit]
            );
            res.json(shifts);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }
}

module.exports = new AnalyticsController();

