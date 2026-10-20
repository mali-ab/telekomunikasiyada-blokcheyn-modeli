-- Schema (matches DatabaseManager::initTables)
CREATE TABLE IF NOT EXISTS blocks (
    id SERIAL PRIMARY KEY,
    prev_hash TEXT NOT NULL,
    block_hash TEXT NOT NULL,
    block_data TEXT NOT NULL,
    timestamp BIGINT NOT NULL,
    nonce INTEGER,
    hash_version SMALLINT NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS subscribers (
    name TEXT PRIMARY KEY CHECK (BTRIM(name) <> ''),
    balance NUMERIC(12, 2) NOT NULL DEFAULT 0 CHECK (balance >= 0),
    internet_expiry TIMESTAMPTZ,
    internet_speed TEXT NOT NULL DEFAULT '1' CHECK (internet_speed IN ('1', '2', '4', '6')),
    iptv_expiry TIMESTAMPTZ,
    iptv_count INTEGER NOT NULL DEFAULT 0 CHECK (iptv_count BETWEEN 0 AND 10),
    phone_expiry TIMESTAMPTZ
);

-- Demo subscribers aligned with the desktop UI mock data
INSERT INTO subscribers (name, balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry)
VALUES
    ('Aman Orazow', 320.00, NOW() + INTERVAL '21 days', '6', NOW() + INTERVAL '21 days', 5, NOW() + INTERVAL '21 days'),
    ('Mähri Döwletowa', 68.00, NOW() + INTERVAL '8 days', '4', NOW() + INTERVAL '8 days', 2, NULL),
    ('Serdar Geldiýew', 0.00, NULL, '1', NULL, 0, NULL),
    ('Aýna Annanepesowa', 145.00, NOW() + INTERVAL '26 days', '2', NOW() + INTERVAL '26 days', 8, NOW() + INTERVAL '26 days')
ON CONFLICT (name) DO NOTHING;
