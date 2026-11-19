'use client';

import React, { useState } from 'react';
import { TelemetryData } from '../../hooks/useAMTTelemetry';
import { triggerEstop, resetSystem } from '../../services/api';
import { ShieldAlert, AlertOctagon, RefreshCw, CheckCircle2, Lock } from 'lucide-react';

interface ViewEmergencyProps {
    telemetry: TelemetryData;
}

export const ViewEmergency: React.FC<ViewEmergencyProps> = ({ telemetry }) => {
    const [overrideConfirmed, setOverrideConfirmed] = useState(false);

    return (
        <div className="space-y-6">
            {/* 1. Giant Emergency Stop Cockpit Button */}
            <div className={`p-8 rounded-3xl border text-center transition-all shadow-2xl ${
                telemetry.emergency
                    ? 'bg-red-950/80 border-red-500 shadow-red-500/20'
                    : 'bg-slate-900/90 border-slate-800'
            }`}>
                <div className="max-w-xl mx-auto flex flex-col items-center">
                    <div className={`w-20 h-20 rounded-full flex items-center justify-center mb-4 ${
                        telemetry.emergency
                            ? 'bg-red-600 text-white animate-bounce'
                            : 'bg-red-600/20 text-red-500 border border-red-500/40'
                    }`}>
                        <AlertOctagon className="w-10 h-10" />
                    </div>

                    <h2 className="text-2xl font-black text-white uppercase tracking-tight">
                        {telemetry.emergency ? 'EMERGENCY STOP IS CURRENTLY ACTIVE' : 'SUPERVISORY EMERGENCY STOP CONTROL'}
                    </h2>

                    <p className="text-xs text-slate-400 mt-2 leading-relaxed">
                        Triggering an emergency stop latches the engine cut relay, immediately halts stepper motor actuation, 
                        and pulls the clutch servo to safe open disengagement. Does not auto-restart upon clearing.
                    </p>

                    <button
                        onClick={() => triggerEstop(!telemetry.emergency)}
                        className={`mt-6 px-10 py-5 rounded-2xl font-black text-lg tracking-wider uppercase transition-all shadow-xl active:scale-95 ${
                            telemetry.emergency
                                ? 'bg-amber-600 hover:bg-amber-500 text-white shadow-amber-600/30 ring-4 ring-amber-500/30'
                                : 'bg-red-600 hover:bg-red-500 text-white shadow-red-600/40 ring-4 ring-red-500/20'
                        }`}
                    >
                        {telemetry.emergency ? 'RESET / CLEAR EMERGENCY STOP' : 'KILL ENTIRE AMT DRIVE SYSTEM'}
                    </button>
                </div>
            </div>

            {/* 2. Safety Interlocks & Recovery Checklist */}
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                        FAILSAFE BEHAVIOR SPECIFICATIONS
                    </span>

                    <div className="space-y-3 text-xs">
                        <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 flex items-start gap-3">
                            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0 mt-0.5" />
                            <div>
                                <strong className="text-white">Clutch Disengage Failsafe:</strong> Servo commands disengagement angle (95°) immediately, isolating engine torque from transmission input shaft.
                            </div>
                        </div>

                        <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 flex items-start gap-3">
                            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0 mt-0.5" />
                            <div>
                                <strong className="text-white">Immediate Stepper Coil Cut:</strong> Halts step pulse generation instantly and de-energizes L298N coils to prevent thermal runaway.
                            </div>
                        </div>

                        <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 flex items-start gap-3">
                            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0 mt-0.5" />
                            <div>
                                <strong className="text-white">Ignition / Fuel Cut Latch:</strong> Optocoupled relay latches into torque-cut state until operator explicitly clears E-Stop.
                            </div>
                        </div>

                        <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 flex items-start gap-3">
                            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0 mt-0.5" />
                            <div>
                                <strong className="text-white">Wi-Fi / MQTT Autonomy:</strong> ESP32 hardware watchdog and pin 35 interrupt operate autonomously even if Wi-Fi or PC crashes.
                            </div>
                        </div>
                    </div>
                </div>

                {/* Controlled System Reboot */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl flex flex-col justify-between">
                    <div>
                        <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                            CONTROLLED RECOVERY & HARDWARE REBOOT
                        </span>
                        <p className="text-xs text-slate-400 leading-relaxed mb-4">
                            In case of hardware latch-up or after resolving mechanical issues, a remote software reboot 
                            can be commanded to reinitialize the ESP32 micro-controller and return transmission to Neutral.
                        </p>

                        <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 mb-4 flex items-center gap-2">
                            <input
                                type="checkbox"
                                id="confirmCheck"
                                checked={overrideConfirmed}
                                onChange={e => setOverrideConfirmed(e.target.checked)}
                                className="w-4 h-4 rounded accent-blue-600"
                            />
                            <label htmlFor="confirmCheck" className="text-xs text-slate-300 font-medium cursor-pointer">
                                I confirm transmission mechanism is physically clear for restart
                            </label>
                        </div>
                    </div>

                    <button
                        onClick={() => {
                            if (overrideConfirmed) {
                                resetSystem();
                            }
                        }}
                        disabled={!overrideConfirmed}
                        className={`w-full py-3 rounded-xl font-bold text-xs flex items-center justify-center gap-2 transition ${
                            overrideConfirmed
                                ? 'bg-blue-600 hover:bg-blue-500 text-white shadow-lg shadow-blue-500/20'
                                : 'bg-slate-800 text-slate-500 cursor-not-allowed border border-slate-700'
                        }`}
                    >
                        <RefreshCw className="w-4 h-4" />
                        REBOOT ESP32 AMT CONTROLLER
                    </button>
                </div>
            </div>
        </div>
    );
};

