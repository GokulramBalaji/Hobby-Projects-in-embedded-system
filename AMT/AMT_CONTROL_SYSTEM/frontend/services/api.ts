// API Client for AMT Backend

const API_BASE = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:5000';

export async function setGear(gear?: number, action?: 'UP' | 'DOWN') {
    const res = await fetch(`${API_BASE}/api/control/gear`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ gear, action })
    });
    return res.json();
}

export async function setMode(mode: 'AUTO' | 'MANUAL') {
    const res = await fetch(`${API_BASE}/api/control/mode`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode })
    });
    return res.json();
}

export async function setClutch(disengage: boolean) {
    const res = await fetch(`${API_BASE}/api/control/clutch`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ disengage })
    });
    return res.json();
}

export async function controlStepper(action: 'JOG' | 'HOME' | 'SET_ZERO' | 'CALIBRATE_GEAR', params: any = {}) {
    const res = await fetch(`${API_BASE}/api/control/stepper`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action, ...params })
    });
    return res.json();
}

export async function triggerEstop(active: boolean) {
    const res = await fetch(`${API_BASE}/api/control/estop`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ active })
    });
    return res.json();
}

export async function resetSystem() {
    const res = await fetch(`${API_BASE}/api/control/reset`, {
        method: 'POST'
    });
    return res.json();
}

export async function toggleSimulation(enabled: boolean) {
    const res = await fetch(`${API_BASE}/api/control/simulation`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled })
    });
    return res.json();
}

export async function fetchLiveTelemetry() {
    const res = await fetch(`${API_BASE}/api/telemetry/live`);
    return res.json();
}

export async function fetchHistory(limit = 100) {
    const res = await fetch(`${API_BASE}/api/telemetry/history?limit=${limit}`);
    return res.json();
}

export async function fetchSummary() {
    const res = await fetch(`${API_BASE}/api/analytics/summary`);
    return res.json();
}

export async function fetchUtilization() {
    const res = await fetch(`${API_BASE}/api/analytics/utilization`);
    return res.json();
}

export async function fetchRpmDistribution() {
    const res = await fetch(`${API_BASE}/api/analytics/rpm-distribution`);
    return res.json();
}

export async function fetchShiftLogs(limit = 50) {
    const res = await fetch(`${API_BASE}/api/analytics/shifts?limit=${limit}`);
    return res.json();
}

export async function fetchFaults() {
    const res = await fetch(`${API_BASE}/api/faults`);
    return res.json();
}

export async function clearFaults() {
    const res = await fetch(`${API_BASE}/api/faults/clear`, { method: 'POST' });
    return res.json();
}

export async function fetchConfig() {
    const res = await fetch(`${API_BASE}/api/config`);
    return res.json();
}

export async function saveConfig(config: any, profileName?: string) {
    const res = await fetch(`${API_BASE}/api/config`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...config, profileName })
    });
    return res.json();
}

export async function listProfiles() {
    const res = await fetch(`${API_BASE}/api/config/profiles`);
    return res.json();
}

export async function restoreProfile(id: number) {
    const res = await fetch(`${API_BASE}/api/config/profiles/${id}/restore`, {
        method: 'POST'
    });
    return res.json();
}

