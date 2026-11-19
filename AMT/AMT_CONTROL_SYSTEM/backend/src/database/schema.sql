-- AMT Control System SQLite Database Schema

CREATE TABLE IF NOT EXISTS devices (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    firmware_version TEXT,
    ip_address TEXT,
    rssi INTEGER,
    status TEXT DEFAULT 'offline',
    last_seen DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS configurations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_name TEXT NOT NULL,
    config_json TEXT NOT NULL,
    is_active INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS rpm_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    instantaneous_rpm REAL NOT NULL,
    average_rpm REAL NOT NULL,
    max_rpm REAL,
    pulse_count INTEGER
);

CREATE TABLE IF NOT EXISTS gear_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    gear INTEGER NOT NULL,
    target_gear INTEGER,
    mode TEXT NOT NULL,
    speed_kmh REAL,
    rpm REAL
);

CREATE TABLE IF NOT EXISTS shift_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    from_gear INTEGER NOT NULL,
    to_gear INTEGER NOT NULL,
    duration_ms INTEGER NOT NULL,
    success INTEGER NOT NULL,
    rpm REAL,
    mode TEXT
);

CREATE TABLE IF NOT EXISTS servo_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    channel INTEGER NOT NULL,
    angle REAL NOT NULL,
    is_disengaged INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS stepper_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    current_position INTEGER NOT NULL,
    target_position INTEGER NOT NULL,
    is_moving INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS fault_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    fault_code INTEGER NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    acknowledged INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS system_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    event_type TEXT NOT NULL,
    source TEXT NOT NULL,
    details TEXT
);

CREATE TABLE IF NOT EXISTS calibration_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    component TEXT NOT NULL,
    parameter_name TEXT NOT NULL,
    value REAL NOT NULL,
    notes TEXT
);

CREATE TABLE IF NOT EXISTS user_sessions (
    id TEXT PRIMARY KEY,
    start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    end_time DATETIME,
    total_shifts INTEGER DEFAULT 0,
    successful_shifts INTEGER DEFAULT 0,
    failed_shifts INTEGER DEFAULT 0,
    max_rpm REAL DEFAULT 0,
    avg_shift_ms REAL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS performance_metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    avg_shift_time_ms REAL,
    success_rate_percent REAL,
    max_recorded_rpm REAL,
    health_score REAL
);

-- Indices for rapid analytical and historical queries
CREATE INDEX IF NOT EXISTS idx_rpm_timestamp ON rpm_readings(timestamp);
CREATE INDEX IF NOT EXISTS idx_shift_timestamp ON shift_events(timestamp);
CREATE INDEX IF NOT EXISTS idx_gear_timestamp ON gear_history(timestamp);
CREATE INDEX IF NOT EXISTS idx_fault_timestamp ON fault_logs(timestamp);

