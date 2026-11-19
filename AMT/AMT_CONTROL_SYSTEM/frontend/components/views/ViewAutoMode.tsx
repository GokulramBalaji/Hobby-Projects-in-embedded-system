'use client';

import React, { useState } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { setMode, saveConfig } from '../../services/api';
import { Settings2, Zap, Check, Gauge } from 'lucide-react';

interface ViewAutoModeProps {
    telemetry: TelemetryData;
}

export const ViewAutoMode: React.FC<ViewAutoModeProps> = ({ telemetry }) => {
    const [autoEnabled, setAutoEnabled] = useState(telemetry.mode === 'AUTO');
    const [thresholds, setThresholds] = useState({
        g1: { up: 3200, down: 1300, speed: 15 },
        g2: { up: 3400, down: 1500, speed: 30 },
        g3: { up: 3600, down: 1700, speed: 50 },
        g4: { up: 4000, down: 1900, speed: 75 }
    });
    const [hysteresis, setHysteresis] = useState(250);
    const [minInterval, setMinInterval] = useState(3000);
    const [engineCutDuration, setEngineCutDuration] = useState(90);
    const [savedMsg, setSavedMsg] = useState(false);

    const handleToggleAuto = async () => {
        const next = !autoEnabled;
        setAutoEnabled(next);
        await setMode(next ? 'AUTO' : 'MANUAL');
    };

    const handleSaveAutoSettings = async () => {
        await saveConfig({
            autoShift: {
                enabled: autoEnabled,
                rpmThresholds: {
                    gear1: { upshiftRpm: thresholds.g1.up, downshiftRpm: thresholds.g1.down, speedKmh: thresholds.g1.speed },
                    gear2: { upshiftRpm: thresholds.g2.up, downshiftRpm: thresholds.g2.down, speedKmh: thresholds.g2.speed },
                    gear3: { upshiftRpm: thresholds.g3.up, downshiftRpm: thresholds.g3.down, speedKmh: thresholds.g3.speed },
                    gear4: { upshiftRpm: thresholds.g4.up, downshiftRpm: thresholds.g4.down, speedKmh: thresholds.g4.speed }
                },
                hysteresisRpm: hysteresis
            },
            shiftSequencing: {
                minimumShiftIntervalMs: minInterval
            },
            engineRelay: {
                cutDurationMs: engineCutDuration
            }
        });
        setSavedMsg(true);
        setTimeout(() => setSavedMsg(false), 2500);
    };

    return (
        <div className="space-y-6">
            {/* 1. Header Banner & Auto Mode Toggle */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-wrap items-center justify-between gap-4">
                <div>
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                        <Zap className="w-4 h-4 text-blue-400" />
                        INTELLIGENT AUTOMATIC GEAR SHIFTING CONTROLLER
                    </span>
                    <div className="text-xl font-black text-white mt-1">
                        RPM-Driven Shift Scheduling & Hysteresis Logic
                    </div>
                </div>

                <div className="flex items-center gap-4">
                    <button
                        onClick={handleToggleAuto}
                        className={`px-6 py-3 rounded-xl font-black text-xs uppercase tracking-wider border transition-all ${
                            autoEnabled
                                ? 'bg-blue-600 hover:bg-blue-500 text-white border-blue-400 shadow-lg shadow-blue-500/25'
                                : 'bg-slate-800 hover:bg-slate-700 text-slate-300 border-slate-700'
                        }`}
                    >
                        AUTOMATIC SHIFTING: {autoEnabled ? 'ENABLED' : 'DISABLED'}
                    </button>
                </div>
            </div>

            {/* 2. Gear Shift Thresholds Grid */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                {[
                    { key: 'g1', title: '1st Gear', speed: 15 },
                    { key: 'g2', title: '2nd Gear', speed: 30 },
                    { key: 'g3', title: '3rd Gear', speed: 50 },
                    { key: 'g4', title: '4th Gear', speed: 75 },
                ].map(({ key, title, speed }) => {
                    const t = (thresholds as any)[key];
                    return (
                        <div key={key} className="p-5 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between shadow-lg">
                            <div className="flex items-center justify-between mb-3">
                                <span className="text-sm font-black text-white">{title}</span>
                                <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-slate-800 text-cyan-400 border border-slate-700">
                                    {speed} KM/H
                                </span>
                            </div>

                            <div className="space-y-3 my-2">
                                <div>
                                    <label className="text-[11px] font-bold text-blue-400 flex justify-between">
                                        <span>Upshift RPM:</span>
                                        <span className="font-mono text-white">{t.up}</span>
                                    </label>
                                    <input
                                        type="number"
                                        step="50"
                                        value={t.up}
                                        onChange={e => {
                                            const val = parseInt(e.target.value, 10) || 0;
                                            setThresholds(prev => ({
                                                ...prev,
                                                [key]: { ...t, up: val }
                                            }));
                                        }}
                                        className="w-full bg-slate-950 border border-slate-800 rounded-lg px-2.5 py-1.5 text-xs text-white font-mono mt-1"
                                    />
                                </div>

                                <div>
                                    <label className="text-[11px] font-bold text-amber-400 flex justify-between">
                                        <span>Downshift RPM:</span>
                                        <span className="font-mono text-white">{t.down}</span>
                                    </label>
                                    <input
                                        type="number"
                                        step="50"
                                        value={t.down}
                                        onChange={e => {
                                            const val = parseInt(e.target.value, 10) || 0;
                                            setThresholds(prev => ({
                                                ...prev,
                                                [key]: { ...t, down: val }
                                            }));
                                        }}
                                        className="w-full bg-slate-950 border border-slate-800 rounded-lg px-2.5 py-1.5 text-xs text-white font-mono mt-1"
                                    />
                                </div>
                            </div>

                            <div className="text-[10px] text-slate-500 mt-2">
                                Delta Band: {t.up - t.down} RPM
                            </div>
                        </div>
                    );
                })}
            </div>

            {/* 3. Global Hysteresis & Timing Parameters */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                <div className="flex items-center justify-between mb-4">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                        <Settings2 className="w-4 h-4 text-cyan-400" />
                        GLOBAL SHIFT TIMING & HYSTERESIS PROTECTION
                    </span>
                    {savedMsg && (
                        <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                            <Check className="w-4 h-4" /> Parameters Applied
                        </span>
                    )}
                </div>

                <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            RPM Hysteresis Margin
                        </label>
                        <input
                            type="number"
                            min="50"
                            max="800"
                            step="25"
                            value={hysteresis}
                            onChange={e => setHysteresis(parseInt(e.target.value, 10) || 250)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Deadband buffer preventing gear hunting near boundary RPM
                        </span>
                    </div>

                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            Minimum Shift Interval (ms)
                        </label>
                        <input
                            type="number"
                            min="1000"
                            max="8000"
                            step="250"
                            value={minInterval}
                            onChange={e => setMinInterval(parseInt(e.target.value, 10) || 3000)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Cooldown duration required between consecutive gear shifts
                        </span>
                    </div>

                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            Ignition Cut Duration (ms)
                        </label>
                        <input
                            type="number"
                            min="40"
                            max="300"
                            step="10"
                            value={engineCutDuration}
                            onChange={e => setEngineCutDuration(parseInt(e.target.value, 10) || 90)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Torque relief window during gear dog engagement
                        </span>
                    </div>
                </div>

                <button
                    onClick={handleSaveAutoSettings}
                    className="mt-6 px-6 py-2.5 bg-blue-600 hover:bg-blue-500 text-white rounded-xl text-xs font-bold transition shadow-lg shadow-blue-500/20"
                >
                    Save Auto Shift Thresholds
                </button>
            </div>
        </div>
    );
};

