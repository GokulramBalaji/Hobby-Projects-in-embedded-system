const express = require('express');
const cors = require('cors');
const http = require('http');
const path = require('path');
require('dotenv').config();

const db = require('./database/db');
const mqttHandler = require('./mqtt/mqttHandler');
const websocketService = require('./services/websocketService');
const hardwareSimulator = require('./simulation/hardwareSimulator');

const controlController = require('./controllers/controlController');
const telemetryController = require('./controllers/telemetryController');
const analyticsController = require('./controllers/analyticsController');
const configController = require('./controllers/configController');
const exportController = require('./controllers/exportController');

const app = express();
const server = http.createServer(app);

const PORT = process.env.PORT || 5000;
const MQTT_BROKER_URL = process.env.MQTT_URL || 'mqtt://127.0.0.1:1883';

// Middleware
app.use(cors());
app.use(express.json());

// Initialize WebSockets
websocketService.init(server);
mqttHandler.setWebSocketService(websocketService);

// ----------------------------------------------------------------------------
// REST API ROUTES
// ----------------------------------------------------------------------------

// 1. Control & Actuators
app.post('/api/control/gear', (req, res) => controlController.setGear(req, res));
app.post('/api/control/mode', (req, res) => controlController.setMode(req, res));
app.post('/api/control/clutch', (req, res) => controlController.setClutch(req, res));
app.post('/api/control/stepper', (req, res) => controlController.controlStepper(req, res));
app.post('/api/control/estop', (req, res) => controlController.emergencyStop(req, res));
app.post('/api/control/reset', (req, res) => controlController.resetSystem(req, res));
app.post('/api/control/simulation', (req, res) => controlController.toggleSimulation(req, res));

// 2. Telemetry & Hardware Status
app.get('/api/telemetry/live', (req, res) => telemetryController.getLive(req, res));
app.get('/api/telemetry/history', (req, res) => telemetryController.getHistory(req, res));
app.get('/api/devices', (req, res) => telemetryController.getDevices(req, res));
app.get('/api/faults', (req, res) => telemetryController.getFaults(req, res));
app.post('/api/faults/clear', (req, res) => telemetryController.clearFaults(req, res));

// 3. Analytics
app.get('/api/analytics/summary', (req, res) => analyticsController.getSummary(req, res));
app.get('/api/analytics/utilization', (req, res) => analyticsController.getGearUtilization(req, res));
app.get('/api/analytics/rpm-distribution', (req, res) => analyticsController.getRpmDistribution(req, res));
app.get('/api/analytics/shifts', (req, res) => analyticsController.getShiftLogs(req, res));

// 4. Configuration
app.get('/api/config', (req, res) => configController.getConfig(req, res));
app.post('/api/config', (req, res) => configController.saveConfig(req, res));
app.get('/api/config/profiles', (req, res) => configController.listProfiles(req, res));
app.post('/api/config/profiles/:id/restore', (req, res) => configController.restoreProfile(req, res));

// 5. Data Export
app.get('/api/export/shifts/csv', (req, res) => exportController.exportShiftsCsv(req, res));
app.get('/api/export/telemetry/csv', (req, res) => exportController.exportTelemetryCsv(req, res));
app.get('/api/export/faults/csv', (req, res) => exportController.exportFaultsCsv(req, res));

// Health check endpoint
app.get('/api/health', (req, res) => {
    res.json({
        status: 'online',
        uptime: process.uptime(),
        mqttConnected: mqttHandler.isConnected,
        wsClients: websocketService.getClientCount(),
        simulationRunning: hardwareSimulator.running
    });
});

// ----------------------------------------------------------------------------
// SERVER INITIALIZATION
// ----------------------------------------------------------------------------
async function startServer() {
    try {
        console.log('====================================================');
        console.log('  AMT CONTROL SYSTEM — INDUSTRIAL PC BACKEND');
        console.log('====================================================');

        // 1. Initialize SQLite Database
        await db.initDatabase();

        // 2. Start MQTT Handler (with embedded Aedes fallback)
        await mqttHandler.start(MQTT_BROKER_URL);

        // 3. Start HTTP & WebSocket Server
        server.listen(PORT, () => {
            console.log(`[HTTP] Express API server listening on http://localhost:${PORT}`);
            console.log(`[WS] Real-time telemetry WebSocket ready on ws://localhost:${PORT}/ws`);

            // By default, start simulator if requested or keep ready
            if (process.env.START_SIMULATOR === 'true' || process.argv.includes('--simulate')) {
                hardwareSimulator.start();
            }
        });

    } catch (err) {
        console.error('[FATAL] Failed to initialize backend server:', err);
        process.exit(1);
    }
}

startServer();

