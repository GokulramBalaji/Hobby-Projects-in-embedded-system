'use client';

import { useState, useEffect, useRef } from 'react';

export interface TelemetryData {
    rpm: number;
    avg_rpm: number;
    max_rpm: number;
    gear: number;
    target_gear: number;
    shift_state?: number;
    mode: 'AUTO' | 'MANUAL';
    clutch_angle: number;
    clutch_disengaged: boolean;
    stepper_position: number;
    stepper_moving: boolean;
    engine_cut: boolean;
    emergency: boolean;
    speed_kmh: number;
    total_shifts: number;
    device_online: boolean;
    timestamp: number;
}

export function useAMTTelemetry() {
    const [telemetry, setTelemetry] = useState<TelemetryData>({
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
        device_online: false,
        timestamp: Date.now()
    });

    const [connected, setConnected] = useState(false);
    const [latency, setLatency] = useState(0);
    const [rpmHistory, setRpmHistory] = useState<{ time: string; rpm: number }[]>([]);
    const wsRef = useRef<WebSocket | null>(null);

    useEffect(() => {
        let isMounted = true;
        let reconnectTimeout: any = null;
        let pingInterval: any = null;

        const connect = () => {
            const wsUrl = process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:5000/ws';
            const ws = new WebSocket(wsUrl);
            wsRef.current = ws;

            ws.onopen = () => {
                if (!isMounted) return;
                setConnected(true);
                // Ping every 3 seconds for latency check
                pingInterval = setInterval(() => {
                    if (ws.readyState === WebSocket.OPEN) {
                        ws.send(JSON.stringify({ type: 'ping', time: Date.now() }));
                    }
                }, 3000);
            };

            ws.onmessage = (event) => {
                if (!isMounted) return;
                try {
                    const msg = JSON.parse(event.data);

                    if (msg.type === 'pong') {
                        const rtt = Date.now() - msg.client_time;
                        setLatency(Math.max(1, rtt));
                    } else if (msg.type === 'mqtt_message' && msg.payload) {
                        if (msg.payload.rpm !== undefined) {
                            const data = msg.payload as TelemetryData;
                            setTelemetry(prev => ({
                                ...prev,
                                ...data,
                                device_online: true
                            }));

                            // Append to RPM history
                            const timeStr = new Date().toLocaleTimeString().split(' ')[0];
                            setRpmHistory(prev => {
                                const next = [...prev, { time: timeStr, rpm: data.rpm }];
                                return next.length > 40 ? next.slice(-40) : next;
                            });
                        }
                    } else if (msg.type === 'device_status') {
                        setTelemetry(prev => ({ ...prev, device_online: msg.payload.online }));
                    }
                } catch (e) {
                    // Ignore parse errors
                }
            };

            ws.onclose = () => {
                if (!isMounted) return;
                setConnected(false);
                if (pingInterval) clearInterval(pingInterval);
                reconnectTimeout = setTimeout(connect, 2000);
            };

            ws.onerror = () => {
                ws.close();
            };
        };

        connect();

        return () => {
            isMounted = false;
            if (pingInterval) clearInterval(pingInterval);
            if (reconnectTimeout) clearTimeout(reconnectTimeout);
            if (wsRef.current) wsRef.current.close();
        };
    }, []);

    return { telemetry, connected, latency, rpmHistory };
}

