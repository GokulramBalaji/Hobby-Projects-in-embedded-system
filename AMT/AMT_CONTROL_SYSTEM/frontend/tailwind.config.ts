import type { Config } from "tailwindcss";

const config: Config = {
  darkMode: "class",
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        amt: {
          bg: "#090d16",
          card: "#111827",
          border: "#1f293d",
          accent: "#3b82f6",
          redline: "#ef4444",
          success: "#10b981",
          warning: "#f59e0b",
          cyan: "#06b6d4",
          purple: "#8b5cf6"
        }
      },
      fontFamily: {
        mono: ['ui-monospace', 'SFMono-Regular', 'Menlo', 'Monaco', 'Consolas', 'monospace'],
      }
    },
  },
  plugins: [],
};

export default config;

