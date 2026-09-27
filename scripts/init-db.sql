-- Schema (matches DatabaseManager::initTables)
CREATE TABLE IF NOT EXISTS blocks (
    id SERIAL PRIMARY KEY,
    prev_hash TEXT,
    block_hash TEXT,
    block_data TEXT,
    timestamp BIGINT
);

CREATE TABLE IF NOT EXISTS subscribers (
    name TEXT PRIMARY KEY,
    balance NUMERIC(10, 2),
    internet_expiry TEXT,
    internet_speed TEXT,
    iptv_expiry TEXT,
    iptv_count TEXT,
    phone_expiry TEXT
);

-- Demo subscribers aligned with the desktop UI mock data
INSERT INTO subscribers (name, balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry)
VALUES
    ('Aman Orazow', 320.00, '2025-09-10 10:42:00', '6', '2025-09-10 10:42:00', '5', '2025-09-10 10:42:00'),
    ('Mähri Döwletowa', 68.00, '2025-09-08 09:15:00', '4', '2025-09-08 09:15:00', '2', 'Inactive'),
    ('Serdar Geldiýew', 0.00, 'Inactive', '1', 'Inactive', '0', 'Inactive'),
    ('Aýna Annanepesowa', 145.00, '2025-09-12 14:20:00', '2', '2025-09-12 14:20:00', '8', '2025-09-12 14:20:00')
ON CONFLICT (name) DO NOTHING;
