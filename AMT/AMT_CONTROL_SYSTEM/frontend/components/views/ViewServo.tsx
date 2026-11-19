'use client';

import React, { useState } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { setClutch, saveConfig } from '../../services/api';
import { Disc, Sliders, ShieldAlert, Check } from 'lucide-react';

interface ViewServoProps {
    telemetry: TelemetryData;
}

export const ViewServo: React.FC<ViewServoProps> = ({ telemetry }) => {
    const [sliderAngle, setSliderAngle] = useState(telemetry.clutch_angle || 15);
    const [engagedCal, setEngagedCal] = useState(15);
    const [disengagedCal, setDisengagedCal] = useState(95);
    const [savedMsg, setSavedMsg] = useState(false);

    const handleSaveCalibration = async () => {
        await saveConfig({
            clutchServo: {
                engagedAngle: engagedCal,
                disengagedAngle: disengagedCal
            }
        });
        setSavedMsg(true);
        setTimeout(() => setSavedMsg(false), 2500);
    };

    return (
        <div className="space-y-6">
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. Clutch Servo Active Position Card */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col justify-between">
                    <div>
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                                <Disc className="w-4 h-4 text-blue-400" />
                                CLUTCH ACTUATOR (PCA9685 CHANNEL 0)
                            </span>
                            <span className={`text-xs font-bold px-2.5 py-0.5 rounded-full ${
                                telemetry.clutch_disengaged
                                    ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30'
                                    : 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                            }`}>
                                {telemetry.clutch_disengaged ? 'CLUTCH DISENGAGED (PULLED)' : 'CLUTCH ENGAGED (CLOSED)'}
                            </span>
                        </div>

                        {/* Angle Gauge Visualizer */}
                        <div className="p-6 bg-slate-950 rounded-2xl border border-slate-800 text-center my-4">
                            <div className="text-6xl font-black text-white font-mono">
                                {telemetry.clutch_angle}°
                            </div>
                            <div className="text-xs text-slate-400 font-semibold uppercase mt-1">
                                Calibrated Angular Displacement
                            </div>
                            <div className="w-full bg-slate-800 h-2 rounded-full overflow-hidden mt-4 max-w-sm mx-auto">
                                <div
                                    className="bg-blue-500 h-full transition-all duration-300"
                                    style={{ width: `${(telemetry.clutch_angle / 180) * 100}%` }}
                                />
                            </div>
                        </div>

                        {/* Quick Test Actions */}
                        <div className="grid grid-cols-2 gap-4 mt-4">
                            <button
                                onClick={() => setClutch(false)}
                                className="py-3 px-4 bg-emerald-600 hover:bg-emerald-500 text-white rounded-xl font-bold text-xs shadow-lg shadow-emerald-500/20 active:scale-95 transition"
                            >
                                ENGAGE CLUTCH (15°)
                            </button>
                            <button
                                onClick={() => setClutch(true)}
                                className="py-3 px-4 bg-amber-600 hover:bg-amber-500 text-white rounded-xl font-bold text-xs shadow-lg shadow-amber-500/20 active:scale-95 transition"
                            >
                                DISENGAGE CLUTCH (95°)
                            </button>
                        </div>
                    </div>

                    <div className="mt-4 pt-4 border-t border-slate-800 text-[11px] text-slate-400">
                        Drive Mechanism: 25 kg·cm high-torque metal gear digital servo with dual ball bearings.
                    </div>
                </div>

                {/* 2. Clutch Calibration Controls */}
                <div className="lg:col-span-6 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col justify-between">
                    <div>
                        <div className="flex items-center justify-between mb-4">
                            <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                                <Sliders className="w-4 h-4 text-cyan-400" />
                                MECHANICAL THROW & ANGLE CALIBRATION
                            </span>
                            {savedMsg && (
                                <span className="text-xs font-bold text-emerald-400 flex items-center gap-1">
                                    <Check className="w-4 h-4" /> Flash Saved
                                </span>
                            )}
                        </div>

                        <div className="space-y-4 my-2">
                            <div>
                                <div className="flex justify-between text-xs font-semibold text-slate-300 mb-1">
                                    <span>Clutch Engaged Rest Angle:</span>
                                    <span className="font-mono text-cyan-400">{engagedCal}°</span>
                                </div>
                                <input
                                    type="range"
                                    min="0"
                                    max="60"
                                    value={engagedCal}
                                    onChange={e => setEngagedCal(parseInt(e.target.value, 10))}
                                    className="w-full accent-blue-500"
                                />
                                <span className="text-[10px] text-slate-500">
                                    Resting position with zero preload on release bearing (typically 10°-25°)
                                </span>
                            </div>

                            <div>
                                <div className="flex justify-between text-xs font-semibold text-slate-300 mb-1">
                                    <span>Clutch Disengaged Pull Angle:</span>
                                    <span className="font-mono text-cyan-400">{disengagedCal}°</span>
                                </div>
                                <input
                                    type="range"
                                    min="60"
                                    max="160"
                                    value={disengagedCal}
                                    onChange={e => setDisengagedCal(parseInt(e.target.value, 10))}
                                    className="w-full accent-blue-500"
                                />
                                <span className="text-[10px] text-slate-500">
                                    Full pull stroke providing clean clutch plate separation (typically 85°-110°)
                                </span>
                            </div>
                        </div>

                        <button
                            onClick={handleSaveCalibration}
                            className="mt-6 w-full py-3 bg-blue-600 hover:bg-blue-500 text-white rounded-xl font-bold text-xs shadow-lg shadow-blue-500/20 transition"
                        >
                            Save Calibration to ESP32 Flash
                        </button>
                    </div>

                    {/* Critical Electrical Engineering Warning */}
                    <div className="mt-4 p-3.5 bg-red-950/30 border border-red-500/30 rounded-xl flex items-start gap-3">
                        <ShieldAlert className="w-5 h-5 text-red-400 shrink-0 mt-0.5" />
                        <div className="text-[11px] text-red-300 leading-relaxed">
                            <strong>CRITICAL ELECTRICAL SAFETY:</strong> A 25 kg·cm servo draws up to 2.8A peak stall current. 
                            Never power the PCA9685 V+ pin from the ESP32 5V/VIN header. Power servos strictly via an independent 
                            5V-6V 5A+ external power supply with common ground to the ESP32.
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

