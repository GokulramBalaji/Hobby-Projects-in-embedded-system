'use client';

import React from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { GaugeRpm } from '../GaugeRpm';
import { GearIndicator } from '../GearIndicator';
import { ActuatorStatusCard } from '../ActuatorStatusCard';
import { ShiftSequenceVisualizer } from '../ShiftSequenceVisualizer';
import { setGear, setMode, triggerEstop } from '../../services/api';
import { ArrowUp, ArrowDown, Power, ShieldAlert } from 'lucide-react';

interface ViewDashboardProps {
    telemetry: TelemetryData;
}

export const ViewDashboard: React.FC<ViewDashboardProps> = ({ telemetry }) => {
    return (
        <div className="space-y-6">
            {/* Top Gauges Grid */}
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. Tachometer Gauge */}
                <div className="lg:col-span-5 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 flex flex-col items-center justify-center shadow-xl">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 mb-2">
                        ENGINE TACHOMETER (V183 IR SENSOR)
                    </span>
                    <GaugeRpm rpm={telemetry.rpm} maxRpm={8000} redline={6000} size={280} />
                    <div className="flex gap-6 mt-4 text-center">
                        <div>
                            <span className="text-[10px] uppercase font-bold text-slate-500">AVG RPM</span>
                            <div className="text-base font-bold text-slate-200 font-mono">
                                {Math.round(telemetry.avg_rpm)}
                            </div>
                        </div>
                        <div>
                            <span className="text-[10px] uppercase font-bold text-slate-500">MAX PEAK</span>
                            <div className="text-base font-bold text-amber-400 font-mono">
                                {Math.round(telemetry.max_rpm)}
                            </div>
                        </div>
                    </div>
                </div>

                {/* 2. Gear & Velocity Cluster */}
                <div className="lg:col-span-4">
                    <GearIndicator
                        gear={telemetry.gear}
                        targetGear={telemetry.target_gear}
                        mode={telemetry.mode}
                        isShifting={telemetry.stepper_moving || (telemetry.shift_state !== undefined && telemetry.shift_state > 0 && telemetry.shift_state < 9)}
                        speedKmh={telemetry.speed_kmh}
                    />
                </div>

                {/* 3. Quick Transmission Cockpit Controls */}
                <div className="lg:col-span-3 bg-slate-900/80 border border-slate-800 rounded-2xl p-5 flex flex-col justify-between shadow-xl">
                    <div>
                        <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                            COCKPIT CONTROLS
                        </span>
                        <div className="grid grid-cols-2 gap-3 mt-4">
                            <button
                                onClick={() => setGear(undefined, 'UP')}
                                className="py-3 px-4 bg-blue-600 hover:bg-blue-500 text-white rounded-xl font-bold flex flex-col items-center justify-center gap-1 shadow-lg shadow-blue-500/20 active:scale-95 transition"
                            >
                                <ArrowUp className="w-5 h-5" />
                                <span className="text-xs">UPSHIFT</span>
                            </button>
                            <button
                                onClick={() => setGear(undefined, 'DOWN')}
                                className="py-3 px-4 bg-slate-800 hover:bg-slate-700 text-white rounded-xl font-bold flex flex-col items-center justify-center gap-1 border border-slate-700 active:scale-95 transition"
                            >
                                <ArrowDown className="w-5 h-5" />
                                <span className="text-xs">DOWNSHIFT</span>
                            </button>
                        </div>

                        {/* Direct Gear Selection */}
                        <div className="grid grid-cols-5 gap-1.5 mt-3">
                            {[0, 1, 2, 3, 4].map(g => (
                                <button
                                    key={g}
                                    onClick={() => setGear(g)}
                                    className={`py-2 rounded-lg font-bold text-xs border transition ${
                                        telemetry.gear === g
                                            ? 'bg-blue-600 border-blue-400 text-white'
                                            : 'bg-slate-950 border-slate-800 text-slate-400 hover:bg-slate-800'
                                    }`}
                                >
                                    {g === 0 ? 'N' : `G${g}`}
                                </button>
                            ))}
                        </div>
                    </div>

                    <div className="space-y-2 mt-4 pt-4 border-t border-slate-800">
                        {/* Auto/Manual Mode Toggle */}
                        <button
                            onClick={() => setMode(telemetry.mode === 'AUTO' ? 'MANUAL' : 'AUTO')}
                            className={`w-full py-2.5 rounded-xl text-xs font-black uppercase tracking-wider border transition ${
                                telemetry.mode === 'AUTO'
                                    ? 'bg-blue-500/20 border-blue-500 text-blue-300'
                                    : 'bg-amber-500/20 border-amber-500 text-amber-300'
                            }`}
                        >
                            TRANSMISSION MODE: {telemetry.mode}
                        </button>

                        {/* Emergency Button */}
                        <button
                            onClick={() => triggerEstop(!telemetry.emergency)}
                            className={`w-full py-2.5 rounded-xl text-xs font-black uppercase tracking-wider flex items-center justify-center gap-2 border transition ${
                                telemetry.emergency
                                    ? 'bg-emerald-600 hover:bg-emerald-500 text-white border-emerald-400'
                                    : 'bg-red-600 hover:bg-red-500 text-white border-red-500 shadow-lg shadow-red-600/30'
                            }`}
                        >
                            <ShieldAlert className="w-4 h-4" />
                            {telemetry.emergency ? 'RESET / CLEAR E-STOP' : 'EMERGENCY STOP'}
                        </button>
                    </div>
                </div>
            </div>

            {/* Actuators Hardware Health Matrix */}
            <ActuatorStatusCard
                clutchAngle={telemetry.clutch_angle}
                clutchDisengaged={telemetry.clutch_disengaged}
                stepperPosition={telemetry.stepper_position}
                stepperMoving={telemetry.stepper_moving}
                engineCut={telemetry.engine_cut}
                totalShifts={telemetry.total_shifts}
            />

            {/* 10-Step Gear Shift Pipeline Animation */}
            <ShiftSequenceVisualizer
                shiftState={telemetry.shift_state || (telemetry.stepper_moving ? 5 : 0)}
                isShifting={telemetry.stepper_moving || (telemetry.shift_state !== undefined && telemetry.shift_state > 0 && telemetry.shift_state < 9)}
            />
        </div>
    );
};

