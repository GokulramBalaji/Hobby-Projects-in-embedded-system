'use client';

import React, { useState, useEffect } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { setGear, setMode, fetchShiftLogs } from '../../services/api';
import { ArrowUp, ArrowDown, CheckCircle2, XCircle, Clock } from 'lucide-react';

interface ViewTransmissionProps {
    telemetry: TelemetryData;
}

export const ViewTransmission: React.FC<ViewTransmissionProps> = ({ telemetry }) => {
    const [shiftLogs, setShiftLogs] = useState<any[]>([]);

    const loadLogs = async () => {
        try {
            const logs = await fetchShiftLogs(15);
            setShiftLogs(logs);
        } catch (e) {}
    };

    useEffect(() => {
        loadLogs();
        const interval = setInterval(loadLogs, 3000);
        return () => clearInterval(interval);
    }, []);

    const gearLabels = ['Neutral (0)', '1st Gear', '2nd Gear', '3rd Gear', '4th Gear'];

    return (
        <div className="space-y-6">
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. Primary Shift Console */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col justify-between">
                    <div>
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                                TRANSMISSION SHIFT CONSOLE
                            </span>
                            <span className={`text-xs font-black px-2.5 py-0.5 rounded-full ${
                                telemetry.mode === 'AUTO'
                                    ? 'bg-blue-500/10 text-blue-400 border border-blue-500/30'
                                    : 'bg-amber-500/10 text-amber-400 border border-amber-500/30'
                            }`}>
                                {telemetry.mode} MODE
                            </span>
                        </div>

                        {/* Large Gear Display */}
                        <div className="flex items-center justify-center p-6 bg-slate-950 rounded-2xl border border-slate-800 my-4">
                            <div className="text-center">
                                <div className="text-7xl font-black text-white font-mono">
                                    {telemetry.gear === 0 ? 'N' : telemetry.gear}
                                </div>
                                <div className="text-xs font-bold uppercase text-slate-400 mt-1">
                                    {gearLabels[telemetry.gear]}
                                </div>
                            </div>
                        </div>

                        {/* Paddle Shift Controls */}
                        <div className="grid grid-cols-2 gap-4 mt-4">
                            <button
                                onClick={() => setGear(undefined, 'UP')}
                                className="py-4 px-6 bg-blue-600 hover:bg-blue-500 text-white rounded-2xl font-black text-sm flex items-center justify-center gap-2 shadow-lg shadow-blue-500/25 active:scale-95 transition"
                            >
                                <ArrowUp className="w-5 h-5" />
                                UPSHIFT (PADDLE +)
                            </button>
                            <button
                                onClick={() => setGear(undefined, 'DOWN')}
                                className="py-4 px-6 bg-slate-800 hover:bg-slate-700 text-white rounded-2xl font-black text-sm flex items-center justify-center gap-2 border border-slate-700 active:scale-95 transition"
                            >
                                <ArrowDown className="w-5 h-5" />
                                DOWNSHIFT (PADDLE -)
                            </button>
                        </div>

                        {/* Direct Select Grid */}
                        <div className="grid grid-cols-5 gap-2 mt-4">
                            {[0, 1, 2, 3, 4].map(g => (
                                <button
                                    key={g}
                                    onClick={() => setGear(g)}
                                    className={`py-3 rounded-xl font-black text-sm border transition ${
                                        telemetry.gear === g
                                            ? 'bg-blue-600 border-blue-400 text-white shadow-lg shadow-blue-500/30'
                                            : 'bg-slate-950 border-slate-800 text-slate-400 hover:bg-slate-800 hover:text-white'
                                    }`}
                                >
                                    {g === 0 ? 'N' : `G${g}`}
                                </button>
                            ))}
                        </div>
                    </div>

                    <div className="pt-4 mt-4 border-t border-slate-800">
                        <button
                            onClick={() => setMode(telemetry.mode === 'AUTO' ? 'MANUAL' : 'AUTO')}
                            className="w-full py-3 rounded-xl text-xs font-bold uppercase bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 transition"
                        >
                            TOGGLE AUTOMATIC / MANUAL MODE
                        </button>
                    </div>
                </div>

                {/* 2. Shift Safety Interlocks Matrix */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                        SAFETY SUPERVISOR & SHIFT INTERLOCK STATUS
                    </span>

                    <div className="space-y-3">
                        <div className="p-3.5 bg-slate-950 rounded-xl border border-slate-800 flex items-center justify-between">
                            <div>
                                <div className="text-xs font-bold text-white">Shift Overlap Prevention</div>
                                <div className="text-[10px] text-slate-400">Guarantees zero overlapping motor actions</div>
                            </div>
                            <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                <CheckCircle2 className="w-4 h-4" /> ACTIVE
                            </span>
                        </div>

                        <div className="p-3.5 bg-slate-950 rounded-xl border border-slate-800 flex items-center justify-between">
                            <div>
                                <div className="text-xs font-bold text-white">Redline Downshift Inhibit</div>
                                <div className="text-[10px] text-slate-400">Rejects downshifts if projected RPM &gt; 6500</div>
                            </div>
                            <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                <CheckCircle2 className="w-4 h-4" /> ACTIVE
                            </span>
                        </div>

                        <div className="p-3.5 bg-slate-950 rounded-xl border border-slate-800 flex items-center justify-between">
                            <div>
                                <div className="text-xs font-bold text-white">Minimum Shift Interval (3000ms)</div>
                                <div className="text-[10px] text-slate-400">Prevents rapid gear oscillation / hunting</div>
                            </div>
                            <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                <CheckCircle2 className="w-4 h-4" /> LOCKED
                            </span>
                        </div>

                        <div className="p-3.5 bg-slate-950 rounded-xl border border-slate-800 flex items-center justify-between">
                            <div>
                                <div className="text-xs font-bold text-white">Ignition Cut Synchronization</div>
                                <div className="text-[10px] text-slate-400">Relieves torque before dog gear displacement</div>
                            </div>
                            <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                <CheckCircle2 className="w-4 h-4" /> 90ms CUT
                            </span>
                        </div>
                    </div>
                </div>
            </div>

            {/* 3. Shift History Audit Log */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                    RECENT SHIFT EXECUTION LOG
                </span>

                <div className="overflow-x-auto">
                    <table className="w-full text-left text-xs">
                        <thead className="bg-slate-950/70 text-slate-400 uppercase text-[10px] font-bold">
                            <tr>
                                <th className="p-3">Time</th>
                                <th className="p-3">Shift Transition</th>
                                <th className="p-3">Duration</th>
                                <th className="p-3">Engine RPM</th>
                                <th className="p-3">Mode</th>
                                <th className="p-3">Result</th>
                            </tr>
                        </thead>
                        <tbody className="divide-y divide-slate-800 font-mono">
                            {shiftLogs.length === 0 ? (
                                <tr>
                                    <td colSpan={6} className="p-4 text-center text-slate-500">
                                        No gear shifts recorded in current session.
                                    </td>
                                </tr>
                            ) : (
                                shiftLogs.map((log) => (
                                    <tr key={log.id} className="hover:bg-slate-800/40">
                                        <td className="p-3 text-slate-400">{log.timestamp}</td>
                                        <td className="p-3 font-bold text-white">
                                            Gear {log.from_gear} → Gear {log.to_gear}
                                        </td>
                                        <td className="p-3 text-cyan-400">{log.duration_ms} ms</td>
                                        <td className="p-3 text-slate-300">{Math.round(log.rpm)} RPM</td>
                                        <td className="p-3 text-slate-400">{log.mode}</td>
                                        <td className="p-3">
                                            {log.success ? (
                                                <span className="inline-flex items-center gap-1 text-emerald-400 font-bold">
                                                    <CheckCircle2 className="w-3.5 h-3.5" /> SUCCESS
                                                </span>
                                            ) : (
                                                <span className="inline-flex items-center gap-1 text-red-400 font-bold">
                                                    <XCircle className="w-3.5 h-3.5" /> FAILED
                                                </span>
                                            )}
                                        </td>
                                    </tr>
                                ))
                            )}
                        </tbody>
                    </table>
                </div>
            </div>
        </div>
    );
};

