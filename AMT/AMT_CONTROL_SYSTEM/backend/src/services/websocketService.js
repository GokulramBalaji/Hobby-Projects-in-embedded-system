const WebSocket = require('ws');

class WebSocketService {
    constructor() {
        this.wss = null;
        this.clients = new Set();
    }

    init(server) {
        this.wss = new WebSocket.Server({ server, path: '/ws' });

        this.wss.on('connection', (ws, req) => {
            const ip = req.socket.remoteAddress;
            console.log(`[WS] Client connected from ${ip}. Total clients: ${this.wss.clients.size}`);
            this.clients.add(ws);

            // Send initial connection acknowledgement and ping test
            ws.send(JSON.stringify({
                type: 'connection_ack',
                payload: { server_time: Date.now(), status: 'connected' }
            }));

            ws.on('message', (message) => {
                try {
                    const data = JSON.parse(message);
                    if (data.type === 'ping') {
                        ws.send(JSON.stringify({ type: 'pong', client_time: data.time, server_time: Date.now() }));
                    }
                } catch (e) {
                    // Ignore non-json
                }
            });

            ws.on('close', () => {
                this.clients.delete(ws);
                console.log(`[WS] Client disconnected. Total clients: ${this.wss.clients.size}`);
            });

            ws.on('error', (err) => {
                console.error('[WS] Client error:', err.message);
                this.clients.delete(ws);
            });
        });

        console.log('[WS] WebSocket server initialized on path /ws.');
    }

    broadcast(type, payload) {
        if (!this.wss) return;

        const message = JSON.stringify({ type, payload, timestamp: Date.now() });

        for (const client of this.wss.clients) {
            if (client.readyState === WebSocket.OPEN) {
                client.send(message);
            }
        }
    }

    getClientCount() {
        return this.wss ? this.wss.clients.size : 0;
    }
}

module.exports = new WebSocketService();

