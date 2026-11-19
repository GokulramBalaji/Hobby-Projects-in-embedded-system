'use client';

import React, { useState } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { controlStepper, saveConfig } from '../../services/api';
import { Cpu, ArrowLeft, ArrowRight, Home, Check, AlertTriangle } from 'lucide-react';

interface ViewStepperProps {
    telemetry: TelemetryData;
}

export const ViewStepper: React.FC<ViewStepperProps> = ({ telemetry }) => {
    const [gearSteps, setGearSteps] = useState([0, 500, 1000, 1500, 2000]);
    const [savedMsg, setSavedMsg] = useState(false);

    const handleSaveSteps = async () => {
        await saveConfig({
            stepperMotor: {
                gearPositions: {
                    neutral: gearSteps[0],
                    gear1: gearSteps[1],
                    gear2: gearSteps[2],
                    gear3: gearSteps[3],
                    gear4: gearSteps[4]
                }
            }
        });
        setSavedMsg(true);
        setTimeout(() => setSavedMsg(false), 2500);
    };

    return (
        <div className="space-y-6">
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. Stepper Active State & Jog Console */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col justify-between">
                    <div>
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                                <Cpu className="w-4 h-4 text-cyan-400" />
                                NEMA 17 GEAR-SHIFT ACTUATOR (423HT40-1204A)
                            </span>
                            <span className={`text-xs font-bold px-2.5 py-0.5 rounded-full ${
                                telemetry.stepper_moving
                                    ? 'bg-blue-500/10 text-blue-400 border border-blue-500/30 animate-pulse'
                                    : 'bg-slate-800 text-slate-300 border border-slate-700'
                            }`}>
                                {telemetry.stepper_moving ? 'ACTUATOR MOVING' : 'HOLDING POSITION'}
                            </span>
                        </div>

                        {/* Step Position Indicator */}
                        <div className="p-6 bg-slate-950 rounded-2xl border border-slate-800 text-center my-4">
                            <div className="text-6xl font-black text-cyan-400 font-mono">
                                {telemetry.stepper_position}
                            </div>
                            <div className="text-xs text-slate-400 font-semibold uppercase mt-1">
                                Current Encoder / Step Position
                            </div>
                        </div>

                        {/* Jog Step Controls */}
                        <span className="text-xs font-bold text-slate-400 uppercase tracking-wider block mb-2">
                            MANUAL STEP JOGGING CONTROLS
                        </span>
                        <div className="grid grid-cols-4 gap-2">
                            <button
                                onClick={() => controlStepper('JOG', { steps: -100 })}
                                className="py-2.5 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-mono font-bold border border-slate-700 active:scale-95 transition"
                            >
                                -100
                            </button>
                            <button
                                onClick={() => controlStepper('JOG', { steps: -10 })}
                                className="py-2.5 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-mono font-bold border border-slate-700 active:scale-95 transition"
                            >
                                -10
                            </button>
                            <button
                                onClick={() => controlStepper('JOG', { steps: 10 })}
                                className="py-2.5 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-mono font-bold border border-slate-700 active:scale-95 transition"
                            >
                                +10
                            </button>
                            <button
                                onClick={() => controlStepper('JOG', { steps: 100 })}
                                className="py-2.5 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-mono font-bold border border-slate-700 active:scale-95 transition"
                            >
                                +100
                            </button>
                        </div>

                        {/* Homing & Zero Set */}
                        <div className="grid grid-cols-2 gap-3 mt-4">
                            <button
                                onClick={() => controlStepper('HOME')}
                                className="py-3 px-4 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-bold flex items-center justify-center gap-2 border border-slate-700 transition"
                            >
                                <Home className="w-4 h-4 text-cyan-400" />
                                RETURN TO HOME
                            </button>
                            <button
                                onClick={() => controlStepper('SET_ZERO')}
                                className="py-3 px-4 bg-cyan-600 hover:bg-cyan-500 text-white rounded-xl text-xs font-bold flex items-center justify-center gap-2 shadow-lg shadow-cyan-600/20 transition"
                            >
                                SET ZERO (NEUTRAL)
                            </button>
                        </div>
                    </div>
                </div>

                {/* 2. Gear Position Presets Calibration */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col justify-between">
                    <div>
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                                GEAR STEP PRESET CALIBRATION
                            </span>
                            {savedMsg && (
                                <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                    <Check className="w-4 h-4" /> Presets Saved
                                </span>
                            )}
                        </div>

                        <div className="space-y-3">
                            {['Neutral (0)', 'Gear 1', 'Gear 2', 'Gear 3', 'Gear 4'].map((name, idx) => (
                                <div key={idx} className="flex items-center justify-between p-3 bg-slate-950 rounded-xl border border-slate-800">
                                    <span className="text-xs font-bold text-slate-300">{name}</span>
                                    <div className="flex items-center gap-2">
                                        <input
                                            type="number"
                                            value={gearSteps[idx]}
                                            onChange={e => {
                                                const val = parseInt(e.target.value, 10) || 0;
                                                const updated = [...gearSteps];
                                                updated[idx] = val;
                                                setGearSteps(updated);
                                            }}
                                            className="w-24 bg-slate-900 border border-slate-700 rounded-lg px-2 py-1 text-right text-xs font-mono text-cyan-400"
                                        />
                                        <span className="text-[11px] text-slate-500">steps</span>
                                    </div>
                                </div>
                            ))}
                        </div>

                        <button
                            onClick={handleSaveSteps}
                            className="mt-5 w-full py-3 bg-blue-600 hover:bg-blue-500 text-white rounded-xl font-bold text-xs shadow-lg shadow-blue-500/20 transition"
                        >
                            Save Step Presets to ESP32 Flash
                        </button>
                    </div>

                    {/* Driver Engineering Advisory */}
                    <div className="mt-4 p-3.5 bg-amber-950/20 border border-amber-500/30 rounded-xl flex items-start gap-3">
                        <AlertTriangle className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
                        <div className="text-[11px] text-amber-200/90 leading-relaxed">
                            <strong>ENGINEERING NOTICE (L298N vs CHOPPER DRIVER):</strong> The NEMA 17 423HT40 has 
                            low phase resistance (~2.4Ω) and 1.2A rated current. L298N is an unchopped BJT driver that produces 
                            significant heat dissipation (5W+). Firmware automatically de-energizes coils after 400ms idle. 
                            For production, an active chopper driver like <strong>TMC2209</strong> or <strong>A4988</strong> is strongly recommended.
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

