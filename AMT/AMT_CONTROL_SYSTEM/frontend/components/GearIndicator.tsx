'use client';

import React from 'react';

interface GearIndicatorProps {
    gear: number;
    targetGear?: number;
    mode: 'AUTO' | 'MANUAL';
    isShifting?: boolean;
    speedKmh?: number;
}

export const GearIndicator: React.FC<GearIndicatorProps> = ({
    gear,
    targetGear = 0,
    mode,
    isShifting = false,
    speedKmh = 0
}) => {
    const gearLabels = ['N', '1', '2', '3', '4'];
    const activeLabel = gearLabels[gear] || 'N';
    const targetLabel = gearLabels[targetGear] || activeLabel;

    return (
        <div className="flex flex-col items-center justify-center p-6 bg-slate-900/80 border border-slate-800 rounded-2xl shadow-xl relative overflow-hidden">
            {/* Shifting Pulsing Glow Background */}
            {isShifting && (
                <div className="absolute inset-0 bg-blue-500/10 animate-pulse pointer-events-none" />
            )}

            <div className="flex items-center justify-between w-full mb-3 px-2">
                <span className="text-xs uppercase font-bold tracking-wider text-slate-400">
                    TRANSMISSION
                </span>
                <span className={`text-[11px] font-black uppercase px-2.5 py-0.5 rounded-full border ${
                    mode === 'AUTO'
                        ? 'bg-blue-500/10 border-blue-500/40 text-blue-400'
                        : 'bg-amber-500/10 border-amber-500/40 text-amber-400'
                }`}>
                    {mode}
                </span>
            </div>

            {/* Huge Gear Numeral */}
            <div className="relative my-2 flex items-center justify-center w-32 h-32 rounded-full bg-slate-950 border-2 border-slate-800 shadow-inner">
                <span className={`text-6xl font-black tracking-tighter ${
                    isShifting ? 'text-blue-400 animate-bounce' : 'text-white'
                }`}>
                    {activeLabel}
                </span>

                {isShifting && (
                    <span className="absolute bottom-2 text-xs font-bold text-blue-400 uppercase tracking-widest animate-pulse">
                        → {targetLabel}
                    </span>
                )}
            </div>

            {/* Speed Readout */}
            <div className="mt-2 text-center">
                <div className="text-2xl font-black text-slate-200 font-mono">
                    {speedKmh.toFixed(1)} <span className="text-xs font-bold text-slate-500">KM/H</span>
                </div>
                <div className="text-[10px] uppercase tracking-wider text-slate-400 font-semibold">
                    ESTIMATED VEHICLE VELOCITY
                </div>
            </div>

            {/* Gear Selector Tray */}
            <div className="flex gap-2 mt-4">
                {[0, 1, 2, 3, 4].map(g => (
                    <div
                        key={g}
                        className={`w-8 h-8 rounded-lg flex items-center justify-center text-xs font-black transition-all ${
                            gear === g
                                ? 'bg-blue-600 text-white shadow-lg shadow-blue-500/30 ring-2 ring-blue-400'
                                : 'bg-slate-800/60 text-slate-500 border border-slate-700/50'
                        }`}
                    >
                        {gearLabels[g]}
                    </div>
                ))}
            </div>
        </div>
    );
};

