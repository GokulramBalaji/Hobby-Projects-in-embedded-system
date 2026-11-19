'use client';

import React from 'react';
import { Cpu, Disc, Zap, Activity } from 'lucide-react';

interface ActuatorStatusCardProps {
    clutchAngle: number;
    clutchDisengaged: boolean;
    stepperPosition: number;
    stepperMoving: boolean;
    engineCut: boolean;
    totalShifts: number;
}

export const ActuatorStatusCard: React.FC<ActuatorStatusCardProps> = ({
    clutchAngle,
    clutchDisengaged,
    stepperPosition,
    stepperMoving,
    engineCut,
    totalShifts
}) => {
    return (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            {/* 1. Clutch Servo Card */}
            <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between">
                <div className="flex items-center justify-between">
                    <span className="text-xs font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
                        <Disc className="w-4 h-4 text-blue-400" />
                        CLUTCH SERVO
                    </span>
                    <span className={`text-[10px] font-bold px-2 py-0.5 rounded-full ${
                        clutchDisengaged
                            ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30'
                            : 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                    }`}>
                        {clutchDisengaged ? 'DISENGAGED' : 'ENGAGED'}
                    </span>
                </div>
                <div className="my-3">
                    <div className="text-3xl font-black text-white font-mono">
                        {clutchAngle}°
                    </div>
                    <div className="text-[11px] text-slate-400 font-medium">
                        PCA9685 Channel 0 (12-bit PWM)
                    </div>
                </div>
                <div className="w-full bg-slate-800 h-1.5 rounded-full overflow-hidden">
                    <div
                        className="bg-blue-500 h-full transition-all duration-300"
                        style={{ width: `${Math.min(100, (clutchAngle / 100) * 100)}%` }}
                    />
                </div>
            </div>

            {/* 2. Stepper Position Card */}
            <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between">
                <div className="flex items-center justify-between">
                    <span className="text-xs font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
                        <Cpu className="w-4 h-4 text-cyan-400" />
                        STEPPER MOTOR
                    </span>
                    <span className={`text-[10px] font-bold px-2 py-0.5 rounded-full ${
                        stepperMoving
                            ? 'bg-blue-500/10 text-blue-400 border border-blue-500/30 animate-pulse'
                            : 'bg-slate-800 text-slate-400 border border-slate-700'
                    }`}>
                        {stepperMoving ? 'STEPPING' : 'HOLDING'}
                    </span>
                </div>
                <div className="my-3">
                    <div className="text-3xl font-black text-white font-mono">
                        {stepperPosition} <span className="text-xs font-normal text-slate-400">STEPS</span>
                    </div>
                    <div className="text-[11px] text-slate-400 font-medium">
                        NEMA 17 – 423HT40 (L298N 4-Phase)
                    </div>
                </div>
                <div className="w-full bg-slate-800 h-1.5 rounded-full overflow-hidden">
                    <div
                        className="bg-cyan-500 h-full transition-all duration-300"
                        style={{ width: `${Math.min(100, (stepperPosition / 2000) * 100)}%` }}
                    />
                </div>
            </div>

            {/* 3. Engine Torque Cut Card */}
            <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between">
                <div className="flex items-center justify-between">
                    <span className="text-xs font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
                        <Zap className="w-4 h-4 text-amber-400" />
                        ENGINE INTERLOCK
                    </span>
                    <span className={`text-[10px] font-bold px-2 py-0.5 rounded-full ${
                        engineCut
                            ? 'bg-red-500/10 text-red-400 border border-red-500/30 animate-pulse'
                            : 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                    }`}>
                        {engineCut ? 'TORQUE CUT' : 'TORQUE ACTIVE'}
                    </span>
                </div>
                <div className="my-3">
                    <div className="text-3xl font-black text-white font-mono">
                        {engineCut ? 'IGNITION CUT' : 'NORMAL'}
                    </div>
                    <div className="text-[11px] text-slate-400 font-medium">
                        Optocoupled Relay (GPIO 19)
                    </div>
                </div>
                <div className="text-[10px] text-slate-400 flex items-center gap-1">
                    <div className={`w-2 h-2 rounded-full ${engineCut ? 'bg-red-500' : 'bg-emerald-500'}`} />
                    Dog clutch torque relief active during shifts
                </div>
            </div>

            {/* 4. Shift Operations Counter */}
            <div className="p-4 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between">
                <div className="flex items-center justify-between">
                    <span className="text-xs font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
                        <Activity className="w-4 h-4 text-purple-400" />
                        SHIFT CYCLE AUDIT
                    </span>
                    <span className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-purple-500/10 text-purple-400 border border-purple-500/30">
                        SESSION
                    </span>
                </div>
                <div className="my-3">
                    <div className="text-3xl font-black text-white font-mono">
                        {totalShifts}
                    </div>
                    <div className="text-[11px] text-slate-400 font-medium">
                        Total Recorded Shift Sequences
                    </div>
                </div>
                <div className="text-[10px] text-slate-400">
                    Watchdog protected • Overlap prevention enabled
                </div>
            </div>
        </div>
    );
};

