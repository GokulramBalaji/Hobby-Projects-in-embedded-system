const sqlite3 = require('sqlite3').verbose();
const fs = require('fs');
const path = require('path');

const DB_PATH = path.resolve(__dirname, 'amt_database.sqlite');
const SCHEMA_PATH = path.resolve(__dirname, 'schema.sql');

let dbInstance = null;

function getDatabase() {
    if (!dbInstance) {
        dbInstance = new sqlite3.Database(DB_PATH, (err) => {
            if (err) {
                console.error('[DATABASE] Failed to connect to SQLite:', err.message);
            } else {
                console.log(`[DATABASE] SQLite connected at ${DB_PATH}`);
            }
        });
        dbInstance.run('PRAGMA journal_mode = WAL;');
        dbInstance.run('PRAGMA synchronous = NORMAL;');
    }
    return dbInstance;
}

function run(sql, params = []) {
    const db = getDatabase();
    return new Promise((resolve, reject) => {
        db.run(sql, params, function (err) {
            if (err) reject(err);
            else resolve({ lastID: this.lastID, changes: this.changes });
        });
    });
}

function get(sql, params = []) {
    const db = getDatabase();
    return new Promise((resolve, reject) => {
        db.get(sql, params, (err, row) => {
            if (err) reject(err);
            else resolve(row);
        });
    });
}

function all(sql, params = []) {
    const db = getDatabase();
    return new Promise((resolve, reject) => {
        db.all(sql, params, (err, rows) => {
            if (err) reject(err);
            else resolve(rows);
        });
    });
}

async function initDatabase() {
    const schemaSql = fs.readFileSync(SCHEMA_PATH, 'utf8');
    const db = getDatabase();

    return new Promise((resolve, reject) => {
        db.exec(schemaSql, async (err) => {
            if (err) {
                console.error('[DATABASE] Schema initialization error:', err);
                return reject(err);
            }
            console.log('[DATABASE] Tables and indices verified successfully.');

            // Seed default config if empty
            try {
                const existing = await get('SELECT COUNT(*) as count FROM configurations');
                if (existing && existing.count === 0) {
                    const defaultConfigPath = path.resolve(__dirname, '../../../config/default-config.json');
                    if (fs.existsSync(defaultConfigPath)) {
                        const rawConfig = fs.readFileSync(defaultConfigPath, 'utf8');
                        await run(
                            'INSERT INTO configurations (profile_name, config_json, is_active) VALUES (?, ?, ?)',
                            ['Factory Default', rawConfig, 1]
                        );
                        console.log('[DATABASE] Seeded initial Factory Default configuration.');
                    }
                }
            } catch (seedErr) {
                console.warn('[DATABASE] Seed check warning:', seedErr.message);
            }

            resolve();
        });
    });
}

module.exports = {
    getDatabase,
    run,
    get,
    all,
    initDatabase
};

