'use client';

import React from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { Line } from 'react-chartjs-2';
import { Activity, Clock, Cpu, Disc, Zap } from 'lucide-react';

interface ViewTelemetryProps {
    telemetry: TelemetryData;
    rpmHistory: { time: string; rpm: number }[];
    latency: number;
}

export const ViewTelemetry: React.FC<ViewTelemetryProps> = ({
    telemetry,
    rpmHistory,
    latency
}) => {
    const chartData = {
        labels: rpmHistory.map(h => h.time),
        datasets: [
            {
                label: 'RPM',
                data: rpmHistory.map(h => h.rpm),
                borderColor: '#38bdf8',
                backgroundColor: 'rgba(56, 189, 248, 0.1)',
                borderWidth: 2,
                tension: 0.2,
                fill: true
            }
        ]
    };

    return (
        <div className="space-y-6">
            {/* Live Telemetry KPI Cards */}
            <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-3">
                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">ENGINE SPEED</span>
                    <div className="text-2xl font-black text-white font-mono">{Math.round(telemetry.rpm)}</div>
                    <span className="text-[10px] text-cyan-400 font-semibold">RPM</span>
                </div>

                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">ACTIVE GEAR</span>
                    <div className="text-2xl font-black text-white font-mono">{telemetry.gear === 0 ? 'N' : telemetry.gear}</div>
                    <span className="text-[10px] text-blue-400 font-semibold">{telemetry.mode} MODE</span>
                </div>

                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">CLUTCH ANGLE</span>
                    <div className="text-2xl font-black text-white font-mono">{telemetry.clutch_angle}°</div>
                    <span className={`text-[10px] font-semibold ${telemetry.clutch_disengaged ? 'text-amber-400' : 'text-emerald-400'}`}>
                        {telemetry.clutch_disengaged ? 'DISENGAGED' : 'ENGAGED'}
                    </span>
                </div>

                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">STEPPER STEPS</span>
                    <div className="text-2xl font-black text-white font-mono">{telemetry.stepper_position}</div>
                    <span className="text-[10px] text-slate-400 font-semibold">NEMA 17</span>
                </div>

                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">TORQUE RELAY</span>
                    <div className="text-2xl font-black text-white font-mono">{telemetry.engine_cut ? 'CUT' : 'NORM'}</div>
                    <span className={`text-[10px] font-semibold ${telemetry.engine_cut ? 'text-red-400' : 'text-emerald-400'}`}>
                        {telemetry.engine_cut ? 'INTERRUPT' : 'DRIVE ON'}
                    </span>
                </div>

                <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl">
                    <span className="text-[10px] uppercase font-bold text-slate-500">MQTT LATENCY</span>
                    <div className="text-2xl font-black text-cyan-400 font-mono">{latency} ms</div>
                    <span className="text-[10px] text-emerald-400 font-semibold">ROUND TRIP</span>
                </div>
            </div>

            {/* High-Resolution Telemetry Chart */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                <div className="flex items-center justify-between mb-4">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                        <Activity className="w-4 h-4 text-cyan-400" />
                        SYNCHRONIZED TELEMETRY TIME-SERIES
                    </span>
                    <span className="text-xs font-mono text-slate-400">
                        {new Date().toLocaleTimeString()}
                    </span>
                </div>

                <div className="h-72 w-full">
                    <Line
                        data={chartData}
                        options={{
                            responsive: true,
                            maintainAspectRatio: false,
                            scales: {
                                y: {
                                    grid: { color: '#1e293b' },
                                    ticks: { color: '#94a3b8' }
                                },
                                x: {
                                    grid: { display: false },
                                    ticks: { color: '#64748b' }
                                }
                            },
                            plugins: { legend: { display: false } }
                        }}
                    />
                </div>
            </div>
        </div>
    );
};

