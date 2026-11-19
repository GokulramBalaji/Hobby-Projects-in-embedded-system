const mqtt = require('mqtt');
const aedesLib = require('aedes');
const Aedes = aedesLib.Aedes || aedesLib;
const net = require('net');
const topics = require('./topics');
const db = require('../database/db');

class MqttHandler {
    constructor() {
        this.client = null;
        this.aedesBroker = null;
        this.tcpServer = null;
        this.isConnected = false;
        this.wsService = null;
        this.latestTelemetry = {
            rpm: 0,
            avg_rpm: 0,
            max_rpm: 0,
            gear: 0,
            target_gear: 0,
            mode: 'MANUAL',
            clutch_angle: 15,
            clutch_disengaged: false,
            stepper_position: 0,
            stepper_moving: false,
            engine_cut: false,
            emergency: false,
            speed_kmh: 0,
            total_shifts: 0,
            timestamp: Date.now(),
            device_online: false
        };
        this.lastMessageTime = 0;
        this.batchRpmCounter = 0;
    }

    setWebSocketService(wsService) {
        this.wsService = wsService;
    }

    async start(brokerUrl = 'mqtt://127.0.0.1:1883') {
        console.log(`[MQTT] Attempting connection to MQTT Broker at ${brokerUrl}...`);

        try {
            await this.connectClient(brokerUrl);
        } catch (err) {
            console.warn(`[MQTT] Standalone broker unavailable at ${brokerUrl} (${err.message}).`);
            console.log('[MQTT] Launching embedded Aedes MQTT broker on port 1883...');
            await this.startEmbeddedBroker(1883);
            await this.connectClient('mqtt://127.0.0.1:1883');
        }

        // Start offline device monitoring check every 2 seconds
        setInterval(() => this.checkDeviceOnlineStatus(), 2000);
    }

    async startEmbeddedBroker(port = 1883) {
        this.aedesBroker = await Aedes.createBroker();
        return new Promise((resolve) => {
            this.tcpServer = net.createServer(this.aedesBroker.handle.bind(this.aedesBroker));

            this.tcpServer.listen(port, () => {
                console.log(`[MQTT] Embedded Aedes MQTT broker listening on port ${port}.`);
                resolve();
            });

            this.tcpServer.on('error', (err) => {
                console.warn(`[MQTT] Port ${port} occupied, attempting fallback port 1884...`);
                this.tcpServer.listen(1884, () => {
                    console.log(`[MQTT] Embedded Aedes MQTT broker listening on fallback port 1884.`);
                    resolve();
                });
            });
        });
    }

    connectClient(brokerUrl) {
        return new Promise((resolve, reject) => {
            let hasResolved = false;
            const client = mqtt.connect(brokerUrl, {
                clientId: 'AMT_NODE_BACKEND',
                connectTimeout: 3000,
                reconnectPeriod: 2000
            });

            client.on('connect', () => {
                this.isConnected = true;
                this.client = client;
                console.log('[MQTT] Backend client connected to MQTT broker successfully.');

                // Subscribe to all AMT topics
                const subscribeList = Object.values(topics);
                client.subscribe(subscribeList, (err) => {
                    if (err) console.error('[MQTT] Subscription error:', err);
                    else console.log(`[MQTT] Subscribed to topics: ${subscribeList.join(', ')}`);
                });

                if (!hasResolved) {
                    hasResolved = true;
                    resolve();
                }
            });

            client.on('message', (topic, message) => {
                this.handleIncomingMessage(topic, message);
            });

            client.on('error', (err) => {
                if (!hasResolved) {
                    hasResolved = true;
                    client.end(true);
                    reject(err);
                }
            });

            client.on('close', () => {
                this.isConnected = false;
            });
        });
    }

    handleIncomingMessage(topic, message) {
        this.lastMessageTime = Date.now();
        this.latestTelemetry.device_online = true;

        let payload;
        try {
            payload = JSON.parse(message.toString());
        } catch (e) {
            payload = { raw: message.toString() };
        }

        switch (topic) {
            case topics.TELEMETRY:
                this.processTelemetry(payload);
                break;
            case topics.EVENTS:
                this.processEvent(payload);
                break;
            case topics.FAULTS:
                this.processFault(payload);
                break;
            case topics.EMERGENCY:
                this.processEmergency(payload);
                break;
            case topics.STATUS:
                this.processStatus(payload);
                break;
            case topics.HEARTBEAT:
                this.processHeartbeat(payload);
                break;
            default:
                break;
        }

        // Forward live update to WebSocket clients
        if (this.wsService) {
            this.wsService.broadcast('mqtt_message', { topic, payload, timestamp: Date.now() });
        }
    }

    async processTelemetry(data) {
        this.latestTelemetry = {
            ...this.latestTelemetry,
            ...data,
            timestamp: Date.now(),
            device_online: true
        };

        // Downsample DB writes for high-frequency RPM (record every 10th sample to avoid DB churn)
        this.batchRpmCounter++;
        if (this.batchRpmCounter >= 10) {
            this.batchRpmCounter = 0;
            try {
                await db.run(
                    `INSERT INTO rpm_readings (instantaneous_rpm, average_rpm, max_rpm, pulse_count)
                     VALUES (?, ?, ?, ?)`,
                    [data.rpm || 0, data.avg_rpm || 0, data.max_rpm || 0, 0]
                );
            } catch (err) {
                // Ignore transient write error
            }
        }
    }

    async processEvent(data) {
        console.log('[MQTT EVENT]', data);
        if (data.event === 'SHIFT_EVENT') {
            try {
                await db.run(
                    `INSERT INTO shift_events (from_gear, to_gear, duration_ms, success, rpm, mode)
                     VALUES (?, ?, ?, ?, ?, ?)`,
                    [
                        data.from_gear || 0,
                        data.to_gear || 0,
                        data.duration_ms || 0,
                        data.success ? 1 : 0,
                        data.rpm || 0,
                        this.latestTelemetry.mode || 'MANUAL'
                    ]
                );
            } catch (e) {
                console.error('[DB] Failed to log shift event:', e.message);
            }
        }
    }

    async processFault(data) {
        console.warn('[MQTT FAULT]', data);
        try {
            await db.run(
                `INSERT INTO fault_logs (fault_code, name, description, acknowledged)
                 VALUES (?, ?, ?, 0)`,
                [data.code || 99, 'FAULT', data.message || 'System fault', 0]
            );
        } catch (e) {
            console.error('[DB] Failed to log fault:', e.message);
        }
    }

    processEmergency(data) {
        this.latestTelemetry.emergency = !!data.emergency_active;
        console.warn(`[MQTT EMERGENCY] Emergency State changed to: ${data.emergency_active}`);
    }

    async processStatus(data) {
        if (data.device) {
            try {
                await db.run(
                    `INSERT INTO devices (id, name, firmware_version, ip_address, rssi, status, last_seen)
                     VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
                     ON CONFLICT(id) DO UPDATE SET
                        ip_address=excluded.ip_address,
                        rssi=excluded.rssi,
                        status=excluded.status,
                        last_seen=CURRENT_TIMESTAMP`,
                    [data.device, data.device, data.version || '2.4.0', data.ip || '127.0.0.1', data.rssi || 0, data.status || 'online']
                );
            } catch (e) {
                console.error('[DB] Failed to update device status:', e.message);
            }
        }
    }

    processHeartbeat(data) {
        this.latestTelemetry.device_online = true;
        this.latestTelemetry.uptime_sec = data.uptime_sec || 0;
        this.latestTelemetry.free_heap = data.free_heap || 0;
        this.latestTelemetry.wifi_rssi = data.wifi_rssi || 0;
    }

    checkDeviceOnlineStatus() {
        const isOnline = (Date.now() - this.lastMessageTime) < 5000;
        if (this.latestTelemetry.device_online !== isOnline) {
            this.latestTelemetry.device_online = isOnline;
            if (this.wsService) {
                this.wsService.broadcast('device_status', { online: isOnline, timestamp: Date.now() });
            }
        }
    }

    publish(topic, payload) {
        if (!this.client || !this.isConnected) {
            return false;
        }
        const message = typeof payload === 'string' ? payload : JSON.stringify(payload);
        this.client.publish(topic, message);
        return true;
    }

    sendCommand(cmd, params = {}) {
        return this.publish(topics.COMMANDS, { cmd, ...params, timestamp: Date.now() });
    }

    sendConfig(config) {
        return this.publish(topics.CONFIG, config);
    }

    sendEmergency(active, reason = 'Operator triggered via PC') {
        return this.publish(topics.EMERGENCY, { emergency_active: active, reason, timestamp: Date.now() });
    }

    getLatestTelemetry() {
        return this.latestTelemetry;
    }
}

module.exports = new MqttHandler();
