'use client';

import React, { useState } from 'react';
import { useAMTTelemetry } from '../hooks/useAMTTelemetry';
import { Header } from '../components/Header';
import { ViewDashboard } from '../components/views/ViewDashboard';
import { ViewRpmMonitoring } from '../components/views/ViewRpmMonitoring';
import { ViewTransmission } from '../components/views/ViewTransmission';
import { ViewServo } from '../components/views/ViewServo';
import { ViewStepper } from '../components/views/ViewStepper';
import { ViewAutoMode } from '../components/views/ViewAutoMode';
import { ViewTelemetry } from '../components/views/ViewTelemetry';
import { ViewAnalytics } from '../components/views/ViewAnalytics';
import { ViewDiagnostics } from '../components/views/ViewDiagnostics';
import { ViewDataManagement } from '../components/views/ViewDataManagement';
import { ViewConfiguration } from '../components/views/ViewConfiguration';
import { ViewEmergency } from '../components/views/ViewEmergency';

export default function AMTApp() {
    const [activeTab, setActiveTab] = useState('dashboard');
    const { telemetry, connected, latency, rpmHistory } = useAMTTelemetry();

    return (
        <div className="min-h-screen flex flex-col bg-amt-bg selection:bg-blue-600 selection:text-white">
            {/* Header & Nav */}
            <Header
                activeTab={activeTab}
                onSelectTab={setActiveTab}
                connected={connected}
                latency={latency}
                deviceOnline={telemetry.device_online}
                emergency={telemetry.emergency}
            />

            {/* Main Content Area */}
            <main className="flex-1 max-w-7xl w-full mx-auto p-4 md:p-6">
                {activeTab === 'dashboard' && <ViewDashboard telemetry={telemetry} />}
                {activeTab === 'rpm' && <ViewRpmMonitoring telemetry={telemetry} rpmHistory={rpmHistory} />}
                {activeTab === 'transmission' && <ViewTransmission telemetry={telemetry} />}
                {activeTab === 'servo' && <ViewServo telemetry={telemetry} />}
                {activeTab === 'stepper' && <ViewStepper telemetry={telemetry} />}
                {activeTab === 'auto' && <ViewAutoMode telemetry={telemetry} />}
                {activeTab === 'telemetry' && <ViewTelemetry telemetry={telemetry} rpmHistory={rpmHistory} latency={latency} />}
                {activeTab === 'analytics' && <ViewAnalytics />}
                {activeTab === 'diagnostics' && <ViewDiagnostics />}
                {activeTab === 'data' && <ViewDataManagement />}
                {activeTab === 'config' && <ViewConfiguration />}
                {activeTab === 'emergency' && <ViewEmergency telemetry={telemetry} />}
            </main>

            {/* Engineering Footer */}
            <footer className="border-t border-slate-800/80 bg-slate-950/80 py-4 px-6 text-center text-xs text-slate-500 flex flex-wrap items-center justify-between gap-2 max-w-7xl mx-auto w-full">
                <div>
                    ESP32-S 38-Pin Microcontroller • V183 IR Sensor • PCA9685 12-bit PWM • NEMA 17 L298N/Chopper
                </div>
                <div className="font-mono text-[11px] text-slate-400">
                    Firmware v2.4.0 • Mosquitto MQTT • Node.js Backend • SQLite WAL
                </div>
            </footer>
        </div>
    );
}

