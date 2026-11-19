'use client';

import React, { useState, useEffect } from 'react';
import { fetchSummary, fetchUtilization, fetchRpmDistribution } from '../../services/api';
import { Bar, Doughnut } from 'react-chartjs-2';
import {
    Chart as ChartJS,
    ArcElement,
    CategoryScale,
    LinearScale,
    BarElement,
    Tooltip,
    Legend
} from 'chart.js';
import { Award, Clock, TrendingUp, CheckCircle, Flame } from 'lucide-react';

ChartJS.register(ArcElement, CategoryScale, LinearScale, BarElement, Tooltip, Legend);

export const ViewAnalytics: React.FC = () => {
    const [summary, setSummary] = useState<any>({
        total_shifts: 0,
        successful_shifts: 0,
        failed_shifts: 0,
        success_rate: 100,
        avg_duration_ms: 320,
        min_duration_ms: 280,
        max_duration_ms: 390,
        max_recorded_rpm: 0,
        avg_engine_rpm: 0,
        performance_score: 95
    });

    const [utilization, setUtilization] = useState<{ [key: string]: number }>({
        '0': 2, '1': 6, '2': 8, '3': 5, '4': 4
    });

    const [rpmDist, setRpmDist] = useState<any>({
        idle_range: 12,
        eco_range: 35,
        cruise_range: 48,
        power_range: 20,
        high_range: 4
    });

    useEffect(() => {
        const load = async () => {
            try {
                const [s, u, r] = await Promise.all([
                    fetchSummary(),
                    fetchUtilization(),
                    fetchRpmDistribution()
                ]);
                if (s) setSummary(s);
                if (u) setUtilization(u);
                if (r) setRpmDist(r);
            } catch (e) {}
        };
        load();
        const interval = setInterval(load, 5000);
        return () => clearInterval(interval);
    }, []);

    const doughnutData = {
        labels: ['Neutral', '1st Gear', '2nd Gear', '3rd Gear', '4th Gear'],
        datasets: [
            {
                data: [
                    utilization['0'] || 0,
                    utilization['1'] || 0,
                    utilization['2'] || 0,
                    utilization['3'] || 0,
                    utilization['4'] || 0
                ],
                backgroundColor: ['#64748b', '#3b82f6', '#06b6d4', '#10b981', '#f59e0b'],
                borderColor: '#0f172a',
                borderWidth: 2
            }
        ]
    };

    const barData = {
        labels: ['<1500 (Idle)', '1500-2500 (Eco)', '2500-3500 (Cruise)', '3500-4500 (Power)', '>4500 (High)'],
        datasets: [
            {
                label: 'Operating Samples',
                data: [
                    rpmDist.idle_range || 0,
                    rpmDist.eco_range || 0,
                    rpmDist.cruise_range || 0,
                    rpmDist.power_range || 0,
                    rpmDist.high_range || 0
                ],
                backgroundColor: '#3b82f6',
                borderRadius: 8
            }
        ]
    };

    return (
        <div className="space-y-6">
            {/* Top Score & KPI Strip */}
            <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
                {/* Automotive Performance Score */}
                <div className="p-5 bg-gradient-to-br from-blue-950/60 to-slate-900 border border-blue-800/40 rounded-2xl flex items-center justify-between shadow-xl">
                    <div>
                        <span className="text-[10px] uppercase font-bold tracking-wider text-blue-400">
                            AMT PERFORMANCE SCORE
                        </span>
                        <div className="text-4xl font-black text-white font-mono mt-1">
                            {summary.performance_score}<span className="text-base text-blue-400">/100</span>
                        </div>
                        <span className="text-[10px] text-slate-400">ISO 26262 Timing Benchmark</span>
                    </div>
                    <div className="w-12 h-12 rounded-2xl bg-blue-600/20 border border-blue-500/30 flex items-center justify-center">
                        <Award className="w-6 h-6 text-blue-400" />
                    </div>
                </div>

                {/* Average Shift Duration */}
                <div className="p-5 bg-slate-900/80 border border-slate-800 rounded-2xl flex items-center justify-between shadow-xl">
                    <div>
                        <span className="text-[10px] uppercase font-bold tracking-wider text-slate-400">
                            AVERAGE SHIFT TIME
                        </span>
                        <div className="text-4xl font-black text-cyan-400 font-mono mt-1">
                            {summary.avg_duration_ms} <span className="text-sm font-normal text-slate-500">ms</span>
                        </div>
                        <span className="text-[10px] text-slate-400">Min: {summary.min_duration_ms}ms • Max: {summary.max_duration_ms}ms</span>
                    </div>
                    <div className="w-12 h-12 rounded-2xl bg-cyan-600/20 border border-cyan-500/30 flex items-center justify-center">
                        <Clock className="w-6 h-6 text-cyan-400" />
                    </div>
                </div>

                {/* Shift Success Rate */}
                <div className="p-5 bg-slate-900/80 border border-slate-800 rounded-2xl flex items-center justify-between shadow-xl">
                    <div>
                        <span className="text-[10px] uppercase font-bold tracking-wider text-slate-400">
                            SHIFT SUCCESS RATE
                        </span>
                        <div className="text-4xl font-black text-emerald-400 font-mono mt-1">
                            {summary.success_rate}%
                        </div>
                        <span className="text-[10px] text-slate-400">{summary.successful_shifts} Pass / {summary.failed_shifts} Fail</span>
                    </div>
                    <div className="w-12 h-12 rounded-2xl bg-emerald-600/20 border border-emerald-500/30 flex items-center justify-center">
                        <CheckCircle className="w-6 h-6 text-emerald-400" />
                    </div>
                </div>

                {/* Peak Engine RPM */}
                <div className="p-5 bg-slate-900/80 border border-slate-800 rounded-2xl flex items-center justify-between shadow-xl">
                    <div>
                        <span className="text-[10px] uppercase font-bold tracking-wider text-slate-400">
                            PEAK RECORDED RPM
                        </span>
                        <div className="text-4xl font-black text-amber-400 font-mono mt-1">
                            {summary.max_recorded_rpm}
                        </div>
                        <span className="text-[10px] text-slate-400">Avg RPM: {summary.avg_engine_rpm}</span>
                    </div>
                    <div className="w-12 h-12 rounded-2xl bg-amber-600/20 border border-amber-500/30 flex items-center justify-center">
                        <Flame className="w-6 h-6 text-amber-400" />
                    </div>
                </div>
            </div>

            {/* Analytical Charts */}
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
                {/* 1. RPM Histogram Distribution */}
                <div className="lg:col-span-7 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4">
                        ENGINE OPERATING RPM BAND DISTRIBUTION
                    </span>
                    <div className="h-64 w-full">
                        <Bar
                            data={barData}
                            options={{
                                responsive: true,
                                maintainAspectRatio: false,
                                scales: {
                                    y: { grid: { color: '#1e293b' }, ticks: { color: '#94a3b8' } },
                                    x: { grid: { display: false }, ticks: { color: '#64748b' } }
                                },
                                plugins: { legend: { display: false } }
                            }}
                        />
                    </div>
                </div>

                {/* 2. Gear Utilization Doughnut */}
                <div className="lg:col-span-5 bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl flex flex-col items-center">
                    <span className="text-xs uppercase font-bold tracking-wider text-slate-400 block mb-4 self-start">
                        GEAR UTILIZATION RATIO
                    </span>
                    <div className="h-56 w-56 my-auto">
                        <Doughnut
                            data={doughnutData}
                            options={{
                                responsive: true,
                                maintainAspectRatio: false,
                                plugins: { legend: { position: 'bottom', labels: { color: '#94a3b8', font: { size: 10 } } } }
                            }}
                        />
                    </div>
                </div>
            </div>
        </div>
    );
};

