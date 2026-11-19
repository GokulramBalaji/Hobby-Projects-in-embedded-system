'use client';

import React, { useState, useEffect } from 'react';
import { fetchConfig, saveConfig, listProfiles, restoreProfile } from '../../services/api';
import { Sliders, Save, Check, RefreshCw, Cpu, Wifi, Radio } from 'lucide-react';

export const ViewConfiguration: React.FC = () => {
    const [config, setConfig] = useState<any>(null);
    const [profiles, setProfiles] = useState<any[]>([]);
    const [profileName, setProfileName] = useState('Custom Workshop Setup');
    const [savedMsg, setSavedMsg] = useState(false);

    const loadConfigAndProfiles = async () => {
        try {
            const [c, p] = await Promise.all([fetchConfig(), listProfiles()]);
            if (c) setConfig(c);
            if (p) setProfiles(p);
        } catch (e) {}
    };

    useEffect(() => {
        loadConfigAndProfiles();
    }, []);

    const handleSave = async () => {
        if (!config) return;
        await saveConfig(config, profileName);
        setSavedMsg(true);
        setTimeout(() => setSavedMsg(false), 3000);
        await loadConfigAndProfiles();
    };

    const handleRestore = async (id: number) => {
        await restoreProfile(id);
        await loadConfigAndProfiles();
    };

    if (!config) {
        return (
            <div className="p-8 text-center text-slate-400">
                Loading configuration schema from SQLite and ESP32...
            </div>
        );
    }

    return (
        <div className="space-y-6">
            {/* Header & Save Bar */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-wrap items-center justify-between gap-4">
                <div>
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                        <Sliders className="w-4 h-4 text-cyan-400" />
                        GLOBAL SYSTEM CONFIGURATION & HARDWARE PARAMETERS
                    </span>
                    <div className="text-xl font-black text-white mt-1">
                        Non-Volatile Storage (NVS) & MQTT Parameter Sync
                    </div>
                </div>

                <div className="flex items-center gap-3">
                    {savedMsg && (
                        <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                            <Check className="w-4 h-4" /> Broadcast & Saved
                        </span>
                    )}
                    <button
                        onClick={handleSave}
                        className="px-6 py-2.5 bg-blue-600 hover:bg-blue-500 text-white rounded-xl text-xs font-bold flex items-center gap-2 shadow-lg shadow-blue-500/20 transition"
                    >
                        <Save className="w-4 h-4" /> Save Configuration
                    </button>
                </div>
            </div>

            {/* Config Panels */}
            <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                {/* 1. Hardware Pin Layout (ESP32 38-Pin) */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl flex flex-col justify-between">
                    <div>
                        <span className="text-xs font-bold text-white uppercase tracking-wider flex items-center gap-2 mb-3">
                            <Cpu className="w-4 h-4 text-blue-400" />
                            Hardware Pin Assignment (Fixed)
                        </span>

                        <div className="space-y-2 text-xs font-mono">
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">RPM Sensor (V183):</span>
                                <span className="text-cyan-400 font-bold">GPIO 34</span>
                            </div>
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">PCA9685 I2C SDA:</span>
                                <span className="text-cyan-400 font-bold">GPIO 21</span>
                            </div>
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">PCA9685 I2C SCL:</span>
                                <span className="text-cyan-400 font-bold">GPIO 22</span>
                            </div>
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">L298N Coil IN1..IN4:</span>
                                <span className="text-cyan-400 font-bold">25, 26, 27, 14</span>
                            </div>
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">Engine Cut Relay:</span>
                                <span className="text-cyan-400 font-bold">GPIO 19</span>
                            </div>
                            <div className="flex justify-between p-2 bg-slate-950 rounded-lg border border-slate-800">
                                <span className="text-slate-400">Hardware E-Stop Switch:</span>
                                <span className="text-cyan-400 font-bold">GPIO 35</span>
                            </div>
                        </div>
                    </div>
                    <div className="text-[10px] text-slate-500 mt-4">
                        Pin assignments comply with ESP32 bootstrapping rules (GPIO 0, 2, 12, 15 protected).
                    </div>
                </div>

                {/* 2. Wi-Fi Settings */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl">
                    <span className="text-xs font-bold text-white uppercase tracking-wider flex items-center gap-2 mb-4">
                        <Wifi className="w-4 h-4 text-emerald-400" />
                        Wi-Fi Station & AP Setup
                    </span>

                    <div className="space-y-4">
                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                Station Wi-Fi SSID
                            </label>
                            <input
                                type="text"
                                value={config.network?.wifiSsid || ''}
                                onChange={e => setConfig({
                                    ...config,
                                    network: { ...config.network, wifiSsid: e.target.value }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>

                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                Station Wi-Fi Password
                            </label>
                            <input
                                type="password"
                                value={config.network?.wifiPassword || ''}
                                onChange={e => setConfig({
                                    ...config,
                                    network: { ...config.network, wifiPassword: e.target.value }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>

                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                Fallback SoftAP SSID
                            </label>
                            <input
                                type="text"
                                value={config.network?.apSsid || 'AMT-CONTROLLER-AP'}
                                onChange={e => setConfig({
                                    ...config,
                                    network: { ...config.network, apSsid: e.target.value }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>
                    </div>
                </div>

                {/* 3. MQTT Broker Settings */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl">
                    <span className="text-xs font-bold text-white uppercase tracking-wider flex items-center gap-2 mb-4">
                        <Radio className="w-4 h-4 text-purple-400" />
                        Mosquitto MQTT Broker Settings
                    </span>

                    <div className="space-y-4">
                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                Broker Host / IP Address
                            </label>
                            <input
                                type="text"
                                value={config.mqtt?.brokerHost || '127.0.0.1'}
                                onChange={e => setConfig({
                                    ...config,
                                    mqtt: { ...config.mqtt, brokerHost: e.target.value }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>

                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                MQTT Broker Port
                            </label>
                            <input
                                type="number"
                                value={config.mqtt?.brokerPort || 1883}
                                onChange={e => setConfig({
                                    ...config,
                                    mqtt: { ...config.mqtt, brokerPort: parseInt(e.target.value, 10) || 1883 }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>

                        <div>
                            <label className="text-xs text-slate-300 font-semibold block mb-1">
                                Client Identifier
                            </label>
                            <input
                                type="text"
                                value={config.mqtt?.clientId || 'ESP32_AMT_01'}
                                onChange={e => setConfig({
                                    ...config,
                                    mqtt: { ...config.mqtt, clientId: e.target.value }
                                })}
                                className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-xs text-white"
                            />
                        </div>
                    </div>
                </div>
            </div>

            {/* Saved Configuration Profiles */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                    CONFIGURATION PROFILES & RESTORATION
                </span>

                <div className="space-y-2">
                    {profiles.map(p => (
                        <div key={p.id} className="flex items-center justify-between p-3 bg-slate-950 rounded-xl border border-slate-800">
                            <div>
                                <div className="text-xs font-bold text-white">{p.profile_name}</div>
                                <div className="text-[10px] text-slate-400">Created: {p.created_at}</div>
                            </div>
                            <div className="flex items-center gap-3">
                                {p.is_active ? (
                                    <span className="text-xs font-bold text-emerald-400">ACTIVE PROFILE</span>
                                ) : (
                                    <button
                                        onClick={() => handleRestore(p.id)}
                                        className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg text-xs font-bold border border-slate-700"
                                    >
                                        Restore Profile
                                    </button>
                                )}
                            </div>
                        </div>
                    ))}
                </div>
            </div>
        </div>
    );
};

