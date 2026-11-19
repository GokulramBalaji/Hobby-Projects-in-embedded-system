'use client';

import React, { useState } from 'react';
import { 
    Activity, Wifi, WifiOff, AlertOctagon, RefreshCw, 
    Play, Square, Sun, Moon 
} from 'lucide-react';
import { triggerEstop, resetSystem, toggleSimulation } from '../services/api';

interface HeaderProps {
    activeTab: string;
    onSelectTab: (tab: string) => void;
    connected: boolean;
    latency: number;
    deviceOnline: boolean;
    emergency: boolean;
}

export const TABS = [
    { id: 'dashboard', label: '1. Dashboard' },
    { id: 'rpm', label: '2. RPM Monitor' },
    { id: 'transmission', label: '3. Transmission' },
    { id: 'servo', label: '4. Servo Control' },
    { id: 'stepper', label: '5. Stepper Control' },
    { id: 'auto', label: '6. Auto Mode' },
    { id: 'telemetry', label: '7. Telemetry' },
    { id: 'analytics', label: '8. Analytics' },
    { id: 'diagnostics', label: '9. Diagnostics' },
    { id: 'data', label: '10. Data Mgmt' },
    { id: 'config', label: '11. Configuration' },
    { id: 'emergency', label: '12. E-Stop' },
];

export const Header: React.FC<HeaderProps> = ({
    activeTab,
    onSelectTab,
    connected,
    latency,
    deviceOnline,
    emergency
}) => {
    const [simulating, setSimulating] = useState(false);
    const [theme, setTheme] = useState<'dark' | 'light'>('dark');

    const handleToggleSim = async () => {
        const next = !simulating;
        setSimulating(next);
        await toggleSimulation(next);
    };

    const toggleTheme = () => {
        const next = theme === 'dark' ? 'light' : 'dark';
        setTheme(next);
        if (next === 'light') {
            document.documentElement.classList.remove('dark');
        } else {
            document.documentElement.classList.add('dark');
        }
    };

    return (
        <header className="w-full bg-slate-900/95 border-b border-slate-800 sticky top-0 z-50 backdrop-blur">
            {/* Critical Emergency Banner if E-Stop is active */}
            {emergency && (
                <div className="bg-red-600 text-white px-4 py-2 font-black text-center text-sm flex items-center justify-center gap-2 animate-pulse">
                    <AlertOctagon className="w-5 h-5" />
                    EMERGENCY STOP ACTIVE — ALL MOTOR DRIVES DISENGAGED & TORQUE CUT
                </div>
            )}

            <div className="max-w-7xl mx-auto px-4 py-3 flex flex-wrap items-center justify-between gap-4">
                {/* Brand / Title */}
                <div className="flex items-center gap-3">
                    <div className="w-9 h-9 rounded-xl bg-blue-600 flex items-center justify-center shadow-lg shadow-blue-500/30">
                        <Activity className="w-5 h-5 text-white" />
                    </div>
                    <div>
                        <div className="text-base font-black tracking-wider text-white">
                            AMT SUPERVISORY SYSTEM
                        </div>
                        <div className="text-[10px] uppercase tracking-widest text-slate-400 font-semibold">
                            ESP32 IoT 4-Speed Automatic Manual Transmission
                        </div>
                    </div>
                </div>

                {/* Status Badges & Controls */}
                <div className="flex items-center gap-3">
                    {/* Simulator Switch */}
                    <button
                        onClick={handleToggleSim}
                        className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-bold border transition-all ${
                            simulating
                                ? 'bg-amber-500/20 border-amber-500 text-amber-300'
                                : 'bg-slate-800 border-slate-700 text-slate-300 hover:bg-slate-700'
                        }`}
                        title="Simulate hardware kinetics without physical ESP32"
                    >
                        {simulating ? <Square className="w-3.5 h-3.5 text-amber-400" /> : <Play className="w-3.5 h-3.5 text-emerald-400" />}
                        SIMULATOR: {simulating ? 'ON' : 'OFF'}
                    </button>

                    {/* ESP32 Status */}
                    <div className={`flex items-center gap-1.5 px-2.5 py-1 rounded-full text-xs font-bold border ${
                        deviceOnline
                            ? 'bg-emerald-500/10 border-emerald-500/30 text-emerald-400'
                            : 'bg-red-500/10 border-red-500/30 text-red-400'
                    }`}>
                        {deviceOnline ? <Wifi className="w-3 h-3" /> : <WifiOff className="w-3 h-3" />}
                        ESP32: {deviceOnline ? 'ONLINE' : 'OFFLINE'}
                    </div>

                    {/* WebSocket Latency */}
                    <div className="hidden sm:flex items-center gap-1 px-2.5 py-1 rounded-full bg-slate-800 border border-slate-700 text-slate-300 text-xs font-mono font-medium">
                        WS: <span className="text-cyan-400 font-bold">{latency}ms</span>
                    </div>

                    {/* Reset Button */}
                    <button
                        onClick={() => resetSystem()}
                        className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white border border-slate-700 transition"
                        title="Reset ESP32 Controller"
                    >
                        <RefreshCw className="w-4 h-4" />
                    </button>

                    {/* Theme Toggle */}
                    <button
                        onClick={toggleTheme}
                        className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white border border-slate-700 transition"
                        title="Toggle Dark / Light Theme"
                    >
                        {theme === 'dark' ? <Sun className="w-4 h-4 text-amber-400" /> : <Moon className="w-4 h-4" />}
                    </button>
                </div>
            </div>

            {/* Navigation Tabs Bar */}
            <div className="max-w-7xl mx-auto px-4 overflow-x-auto scrollbar-none border-t border-slate-800/80">
                <nav className="flex space-x-1 py-1.5 min-w-max">
                    {TABS.map((t) => (
                        <button
                            key={t.id}
                            onClick={() => onSelectTab(t.id)}
                            className={`px-3 py-1.5 rounded-lg text-xs font-semibold whitespace-nowrap transition-all ${
                                activeTab === t.id
                                    ? 'bg-blue-600 text-white shadow-md shadow-blue-600/30'
                                    : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
                            }`}
                        >
                            {t.label}
                        </button>
                    ))}
                </nav>
            </div>
        </header>
    );
};

