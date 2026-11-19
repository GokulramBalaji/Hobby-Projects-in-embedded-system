const db = require('../database/db');
const mqttHandler = require('../mqtt/mqttHandler');
const fs = require('fs');
const path = require('path');

class ConfigController {
    async getConfig(req, res) {
        try {
            const row = await db.get('SELECT * FROM configurations WHERE is_active = 1 ORDER BY id DESC LIMIT 1');
            if (row && row.config_json) {
                return res.json(JSON.parse(row.config_json));
            }

            // Fallback to default json file
            const defaultPath = path.resolve(__dirname, '../../../config/default-config.json');
            if (fs.existsSync(defaultPath)) {
                const data = JSON.parse(fs.readFileSync(defaultPath, 'utf8'));
                return res.json(data);
            }

            res.status(404).json({ error: 'No configuration found' });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async saveConfig(req, res) {
        const configData = req.body;
        const profileName = req.body.profileName || 'Custom Configuration';

        try {
            const configJson = JSON.stringify(configData, null, 2);

            // Deactivate previous profiles
            await db.run('UPDATE configurations SET is_active = 0');

            // Insert new active profile
            const result = await db.run(
                'INSERT INTO configurations (profile_name, config_json, is_active) VALUES (?, ?, 1)',
                [profileName, configJson]
            );

            // Broadcast to ESP32 over MQTT
            mqttHandler.sendConfig(configData);

            res.json({ status: 'ok', id: result.lastID, message: 'Configuration saved and broadcast to ESP32' });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async listProfiles(req, res) {
        try {
            const profiles = await db.all('SELECT id, profile_name, is_active, created_at FROM configurations ORDER BY id DESC');
            res.json(profiles);
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }

    async restoreProfile(req, res) {
        const { id } = req.params;
        try {
            const row = await db.get('SELECT * FROM configurations WHERE id = ?', [id]);
            if (!row) return res.status(404).json({ error: 'Profile not found' });

            await db.run('UPDATE configurations SET is_active = 0');
            await db.run('UPDATE configurations SET is_active = 1 WHERE id = ?', [id]);

            const configData = JSON.parse(row.config_json);
            mqttHandler.sendConfig(configData);

            res.json({ status: 'ok', profile: row.profile_name, config: configData });
        } catch (err) {
            res.status(500).json({ error: err.message });
        }
    }
}

module.exports = new ConfigController();

