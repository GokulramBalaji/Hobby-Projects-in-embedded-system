'use client';

import React from 'react';
import { Database, Download, FileText, HardDrive, CheckCircle2 } from 'lucide-react';

export const ViewDataManagement: React.FC = () => {
    const API_BASE = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:5000';

    return (
        <div className="space-y-6">
            {/* 1. Database Overview Banner */}
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex items-center justify-between">
                <div>
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 flex items-center gap-2">
                        <Database className="w-4 h-4 text-cyan-400" />
                        SQLITE RELATIONAL STORAGE & TELEMETRY ARCHIVE
                    </span>
                    <div className="text-xl font-black text-white mt-1">
                        High-Performance WAL Engine (Write-Ahead Logging)
                    </div>
                </div>
                <div className="text-right">
                    <span className="text-xs font-bold text-emerald-400 flex items-center justify-end gap-1">
                        <CheckCircle2 className="w-4 h-4" /> ACID COMPLIANT
                    </span>
                    <div className="text-[11px] text-slate-400 mt-0.5">Database: amt_database.sqlite</div>
                </div>
            </div>

            {/* 2. CSV Data Exporters */}
            <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                {/* Shifts CSV Export */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between shadow-xl">
                    <div>
                        <div className="w-10 h-10 rounded-xl bg-blue-600/20 border border-blue-500/30 flex items-center justify-center mb-4">
                            <FileText className="w-5 h-5 text-blue-400" />
                        </div>
                        <h3 className="text-sm font-black text-white mb-1">Shift Events History</h3>
                        <p className="text-xs text-slate-400 leading-relaxed">
                            Export every individual gear shift transition with execution duration, gear delta, RPM, and pass/fail audit flags.
                        </p>
                    </div>
                    <a
                        href={`${API_BASE}/api/export/shifts/csv`}
                        download
                        className="mt-6 w-full py-2.5 bg-blue-600 hover:bg-blue-500 text-white rounded-xl text-xs font-bold flex items-center justify-center gap-2 shadow-lg shadow-blue-500/20 transition"
                    >
                        <Download className="w-4 h-4" /> Download Shifts CSV
                    </a>
                </div>

                {/* RPM Telemetry CSV Export */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between shadow-xl">
                    <div>
                        <div className="w-10 h-10 rounded-xl bg-cyan-600/20 border border-cyan-500/30 flex items-center justify-center mb-4">
                            <HardDrive className="w-5 h-5 text-cyan-400" />
                        </div>
                        <h3 className="text-sm font-black text-white mb-1">RPM Time-Series Data</h3>
                        <p className="text-xs text-slate-400 leading-relaxed">
                            Export high-resolution rotational speed records including instantaneous RPM, moving averages, and optical pulse counts.
                        </p>
                    </div>
                    <a
                        href={`${API_BASE}/api/export/telemetry/csv`}
                        download
                        className="mt-6 w-full py-2.5 bg-cyan-600 hover:bg-cyan-500 text-white rounded-xl text-xs font-bold flex items-center justify-center gap-2 shadow-lg shadow-cyan-500/20 transition"
                    >
                        <Download className="w-4 h-4" /> Download RPM CSV
                    </a>
                </div>

                {/* Faults CSV Export */}
                <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl flex flex-col justify-between shadow-xl">
                    <div>
                        <div className="w-10 h-10 rounded-xl bg-amber-600/20 border border-amber-500/30 flex items-center justify-center mb-4">
                            <Database className="w-5 h-5 text-amber-400" />
                        </div>
                        <h3 className="text-sm font-black text-white mb-1">Diagnostic Fault Logs</h3>
                        <p className="text-xs text-slate-400 leading-relaxed">
                            Export all timestamped supervisor fault codes, watchdog trips, actuator timeouts, and emergency stop occurrences.
                        </p>
                    </div>
                    <a
                        href={`${API_BASE}/api/export/faults/csv`}
                        download
                        className="mt-6 w-full py-2.5 bg-amber-600 hover:bg-amber-500 text-white rounded-xl text-xs font-bold flex items-center justify-center gap-2 shadow-lg shadow-amber-500/20 transition"
                    >
                        <Download className="w-4 h-4" /> Download Faults CSV
                    </a>
                </div>
            </div>
        </div>
    );
};

