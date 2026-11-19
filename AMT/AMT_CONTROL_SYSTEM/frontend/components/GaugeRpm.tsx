'use client';

import React, { useEffect, useRef } from 'react';

interface GaugeRpmProps {
    rpm: number;
    maxRpm?: number;
    redline?: number;
    size?: number;
}

export const GaugeRpm: React.FC<GaugeRpmProps> = ({
    rpm,
    maxRpm = 8000,
    redline = 6000,
    size = 280
}) => {
    const canvasRef = useRef<HTMLCanvasElement | null>(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        const dpr = window.devicePixelRatio || 1;
        canvas.width = size * dpr;
        canvas.height = size * dpr;
        ctx.scale(dpr, dpr);

        const cx = size / 2;
        const cy = size / 2;
        const radius = (size / 2) - 18;

        // Angles in radians: start at 135 deg (bottom left), sweep 270 deg to 45 deg (bottom right)
        const startAngle = (135 * Math.PI) / 180;
        const endAngle = (405 * Math.PI) / 180;
        const totalAngle = endAngle - startAngle;

        ctx.clearRect(0, 0, size, size);

        // 1. Background Arc Track
        ctx.beginPath();
        ctx.arc(cx, cy, radius, startAngle, endAngle);
        ctx.lineWidth = 10;
        ctx.strokeStyle = '#1e293b';
        ctx.stroke();

        // 2. Redline Arc Zone
        const redlineFraction = (redline / maxRpm);
        const redlineStartAngle = startAngle + (totalAngle * redlineFraction);
        ctx.beginPath();
        ctx.arc(cx, cy, radius, redlineStartAngle, endAngle);
        ctx.lineWidth = 10;
        ctx.strokeStyle = 'rgba(239, 68, 68, 0.4)';
        ctx.stroke();

        // 3. Active Value Glow Arc
        const currentFraction = Math.min(1, Math.max(0, rpm / maxRpm));
        const currentAngle = startAngle + (totalAngle * currentFraction);

        if (currentFraction > 0) {
            ctx.beginPath();
            ctx.arc(cx, cy, radius, startAngle, currentAngle);
            ctx.lineWidth = 10;
            const isRedline = rpm >= redline;
            ctx.strokeStyle = isRedline ? '#ef4444' : '#3b82f6';
            ctx.shadowBlur = 12;
            ctx.shadowColor = isRedline ? '#ef4444' : '#3b82f6';
            ctx.stroke();
            ctx.shadowBlur = 0; // reset shadow
        }

        // 4. Tick Marks & Numbers (0 to 8 x1000)
        const ticks = 8;
        for (let i = 0; i <= ticks; i++) {
            const fraction = i / ticks;
            const angle = startAngle + (totalAngle * fraction);
            const isMajor = true;
            const tickR1 = radius - (isMajor ? 12 : 6);
            const tickR2 = radius - 2;

            const x1 = cx + Math.cos(angle) * tickR1;
            const y1 = cy + Math.sin(angle) * tickR1;
            const x2 = cx + Math.cos(angle) * tickR2;
            const y2 = cy + Math.sin(angle) * tickR2;

            const isTickRedline = (i * 1000) >= redline;

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineWidth = isMajor ? 2.5 : 1.5;
            ctx.strokeStyle = isTickRedline ? '#ef4444' : '#64748b';
            ctx.stroke();

            // Labels
            const labelR = radius - 24;
            const lx = cx + Math.cos(angle) * labelR;
            const ly = cy + Math.sin(angle) * labelR + 4;

            ctx.fillStyle = isTickRedline ? '#ef4444' : '#94a3b8';
            ctx.font = '600 11px system-ui, -apple-system, sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText(i.toString(), lx, ly);
        }

        // 5. Needle
        const needleLen = radius - 8;
        const nx = cx + Math.cos(currentAngle) * needleLen;
        const ny = cy + Math.sin(currentAngle) * needleLen;

        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(nx, ny);
        ctx.lineWidth = 3;
        ctx.strokeStyle = rpm >= redline ? '#f87171' : '#60a5fa';
        ctx.shadowBlur = 8;
        ctx.shadowColor = '#60a5fa';
        ctx.stroke();
        ctx.shadowBlur = 0;

        // 6. Center Hub Cap
        ctx.beginPath();
        ctx.arc(cx, cy, 7, 0, Math.PI * 2);
        ctx.fillStyle = '#0f172a';
        ctx.fill();
        ctx.lineWidth = 2;
        ctx.strokeStyle = '#38bdf8';
        ctx.stroke();

    }, [rpm, maxRpm, redline, size]);

    return (
        <div className="relative flex flex-col items-center justify-center">
            <canvas ref={canvasRef} style={{ width: size, height: size }} />
            <div className="absolute flex flex-col items-center mt-12 pointer-events-none">
                <span className="text-3xl font-black tracking-tight text-white font-mono">
                    {Math.round(rpm)}
                </span>
                <span className="text-[10px] uppercase font-bold tracking-widest text-slate-400">
                    RPM × 1000
                </span>
            </div>
        </div>
    );
};

