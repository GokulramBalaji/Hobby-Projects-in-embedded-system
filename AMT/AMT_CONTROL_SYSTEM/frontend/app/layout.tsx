import './globals.css';
import type { Metadata } from 'next';

export const metadata: Metadata = {
  title: 'ESP32 Advanced IoT Automatic Manual Transmission (AMT) System',
  description: 'Industrial Automotive AMT Supervisory Cockpit & Real-Time Telemetry Dashboard',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="bg-amt-bg text-slate-100 min-h-screen">
        {children}
      </body>
    </html>
  );
}

