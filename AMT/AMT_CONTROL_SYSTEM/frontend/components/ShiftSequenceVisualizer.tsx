'use client';

import React from 'react';
import { CheckCircle2, Loader2, AlertCircle } from 'lucide-react';

interface ShiftSequenceVisualizerProps {
    shiftState?: number;
    isShifting: boolean;
}

const SHIFT_STEPS = [
    { id: 1, label: '1. Validate Target' },
    { id: 2, label: '2. Check RPM Limit' },
    { id: 3, label: '3. Pull Clutch' },
    { id: 4, label: '4. Cut Torque' },
    { id: 5, label: '5. Move Stepper' },
    { id: 6, label: '6. Verify Position' },
    { id: 7, label: '7. Restore Torque' },
    { id: 8, label: '8. Release Clutch' },
    { id: 9, label: '9. Confirm Gear' },
    { id: 10, label: '10. Shift Complete' }
];

export const ShiftSequenceVisualizer: React.FC<ShiftSequenceVisualizerProps> = ({
    shiftState = 0,
    isShifting
}) => {
    return (
        <div className="w-full bg-slate-900/90 border border-slate-800 rounded-2xl p-5 shadow-lg">
            <div className="flex items-center justify-between mb-3">
                <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                    AUTOMATIC SHIFT SEQUENCING PIPELINE (10-PHASE INTERLOCK)
                </span>
                <span className={`text-xs font-semibold px-2 py-0.5 rounded-full ${
                    isShifting
                        ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30 animate-pulse'
                        : 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/30'
                }`}>
                    {isShifting ? 'EXECUTING TRANSITION' : 'STANDBY / LOCKED'}
                </span>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-5 md:grid-cols-10 gap-2 mt-3">
                {SHIFT_STEPS.map((step) => {
                    const isActive = isShifting && shiftState === step.id;
                    const isCompleted = !isShifting || shiftState > step.id;

                    return (
                        <div
                            key={step.id}
                            className={`p-2.5 rounded-xl border flex flex-col items-center justify-center text-center transition-all ${
                                isActive
                                    ? 'bg-blue-600/20 border-blue-500 text-blue-300 ring-2 ring-blue-500/40 shadow-lg shadow-blue-500/20'
                                    : isCompleted && isShifting
                                    ? 'bg-emerald-950/20 border-emerald-600/40 text-emerald-400'
                                    : 'bg-slate-950/50 border-slate-800 text-slate-500'
                            }`}
                        >
                            {isActive ? (
                                <Loader2 className="w-4 h-4 animate-spin mb-1 text-blue-400" />
                            ) : isCompleted && isShifting ? (
                                <CheckCircle2 className="w-4 h-4 mb-1 text-emerald-400" />
                            ) : (
                                <div className="w-2 h-2 rounded-full bg-slate-700 mb-2" />
                            )}
                            <span className="text-[10px] font-bold tracking-tight leading-tight">
                                {step.label}
                            </span>
                        </div>
                    );
                })}
            </div>
        </div>
    );
};

