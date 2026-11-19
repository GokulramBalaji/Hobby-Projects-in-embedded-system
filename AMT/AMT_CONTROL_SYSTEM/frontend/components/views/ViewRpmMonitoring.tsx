'use client';

import React, { useState } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { GaugeRpm } from '../GaugeRpm';
import { saveConfig } from '../../services/api';
import { Line } from 'react-chartjs-2';
import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
    Filler
} from 'chart.js';

ChartJS.register(
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
    Filler
);

interface ViewRpmMonitoringProps {
    telemetry: TelemetryData;
    rpmHistory: { time: string; rpm: number }[];
}

export const ViewRpmMonitoring: React.FC<ViewRpmMonitoringProps> = ({
    telemetry,
    rpmHistory
}) => {
    const [ppr, setPpr] = useState(1);
    const [glitchFilter, setGlitchFilter] = useState(200);
    const [calFactor, setCalFactor] = useState(1.0);
    const [savedMsg, setSavedMsg] = useState(false);

    const handleSaveRpmConfig = async () => {
        await saveConfig({
            rpmSensor: {
                pulsesPerRevolution: ppr,
                glitchFilterMicros: glitchFilter,
                calibrationFactor: calFactor
            }
        });
        setSavedMsg(true);
        setTimeout(() => setSavedMsg(false), 2500);
    };

    const chartData = {
        labels: rpmHistory.map(h => h.time),
        datasets: [
            {
                label: 'Engine RPM',
                data: rpmHistory.map(h => h.rpm),
                borderColor: '#3b82f6',
                backgroundColor: 'rgba(59, 130, 246, 0.1)',
                borderWidth: 2,
                pointRadius: 1,
                tension: 0.3,
                fill: true
            }
        ]
    };

    const chartOptions = {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
            y: {
                min: 0,
                max: 7000,
                grid: { color: '#1e293b' },
                ticks: { color: '#94a3b8' }
            },
            x: {
                grid: { display: false },
                ticks: { color: '#64748b', maxTicksLimit: 10 }
            }
        },
        plugins: {
            legend: { display: false }
        }
    };

    return (
        <div className="space-y-6">
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. Gauge & Live Metrics */}
                <div className="lg:col-span-5 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 flex flex-col items-center justify-between shadow-xl">
                    <div className="text-xs uppercase font-bold tracking-wider text-slate-400">
                        V183 IR OPTICAL TACHOMETER
                    </div>
                    <GaugeRpm rpm={telemetry.rpm} maxRpm={8000} redline={6000} size={280} />

                    <div className="grid grid-cols-3 gap-4 w-full mt-4 pt-4 border-t border-slate-800 text-center">
                        <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800">
                            <span className="text-[10px] uppercase font-bold text-slate-500">INSTANT</span>
                            <div className="text-xl font-black text-blue-400 font-mono">
                                {Math.round(telemetry.rpm)}
                            </div>
                        </div>
                        <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800">
                            <span className="text-[10px] uppercase font-bold text-slate-500">AVERAGE</span>
                            <div className="text-xl font-black text-slate-200 font-mono">
                                {Math.round(telemetry.avg_rpm)}
                            </div>
                        </div>
                        <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800">
                            <span className="text-[10px] uppercase font-bold text-slate-500">MAX PEAK</span>
                            <div className="text-xl font-black text-amber-400 font-mono">
                                {Math.round(telemetry.max_rpm)}
                            </div>
                        </div>
                    </div>
                </div>

                {/* 2. Real-time Waveform Chart */}
                <div className="lg:col-span-7 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 flex flex-col shadow-xl">
                    <div className="flex items-center justify-between mb-4">
                        <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                            HIGH-SPEED RPM TELEMETRY STREAM
                        </span>
                        <div className="flex items-center gap-2">
                            <div className="w-2 h-2 rounded-full bg-blue-500 animate-ping" />
                            <span className="text-[11px] text-slate-400 font-medium">10 Hz Sample Rate</span>
                        </div>
                    </div>
                    <div className="h-64 w-full">
                        <Line data={chartData} options={chartOptions} />
                    </div>
                </div>
            </div>

            {/* 3. Sensor Calibration & Noise Filtering Controls */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                <div className="flex items-center justify-between mb-4">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                        SENSOR CALIBRATION & PULSE COUNTING CONFIGURATION
                    </span>
                    {savedMsg && (
                        <span className="text-xs font-bold text-emerald-400">
                            ✓ Calibration parameters dispatched to ESP32
                        </span>
                    )}
                </div>

                <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            Pulses Per Revolution (PPR)
                        </label>
                        <input
                            type="number"
                            min="1"
                            max="60"
                            value={ppr}
                            onChange={e => setPpr(parseInt(e.target.value, 10) || 1)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm focus:border-blue-500 outline-none"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Encoder flag count per 360° rotation (default: 1)
                        </span>
                    </div>

                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            Glitch Filter Window (µs)
                        </label>
                        <input
                            type="number"
                            min="50"
                            max="2000"
                            step="50"
                            value={glitchFilter}
                            onChange={e => setGlitchFilter(parseInt(e.target.value, 10) || 200)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm focus:border-blue-500 outline-none"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Rejects optical reflections/bounce pulses &lt; window
                        </span>
                    </div>

                    <div>
                        <label className="text-xs font-semibold text-slate-300 block mb-1">
                            Software Calibration Factor
                        </label>
                        <input
                            type="number"
                            min="0.5"
                            max="2.0"
                            step="0.05"
                            value={calFactor}
                            onChange={e => setCalFactor(parseFloat(e.target.value) || 1.0)}
                            className="w-full bg-slate-950 border border-slate-800 rounded-xl px-3 py-2 text-white font-mono text-sm focus:border-blue-500 outline-none"
                        />
                        <span className="text-[10px] text-slate-500 mt-1 block">
                            Multiplier factor against stroboscopic reference
                        </span>
                    </div>
                </div>

                <button
                    onClick={handleSaveRpmConfig}
                    className="mt-5 px-6 py-2.5 bg-blue-600 hover:bg-blue-500 text-white rounded-xl text-xs font-bold transition shadow-lg shadow-blue-500/20"
                >
                    Apply RPM Configuration
                </button>
            </div>
        </div>
    );
};

