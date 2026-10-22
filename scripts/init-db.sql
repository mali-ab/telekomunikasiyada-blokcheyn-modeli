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

