#include "database.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace {
std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return first < last ? std::string(first, last) : std::string{};
}

std::string connectionValue(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    for (const char character : value) {
        if (character == '\\' || character == '\'') escaped.push_back('\\');
        escaped.push_back(character);
    }
    return "'" + escaped + "'";
}

std::string columnType(pqxx::work &transaction, const std::string &column) {
    const auto rows = transaction.exec(
        "SELECT data_type FROM information_schema.columns "
        "WHERE table_schema = current_schema() AND table_name = 'subscribers' AND column_name = " +
        transaction.quote(column));
    return rows.empty() ? std::string{} : rows[0][0].as<std::string>();
}

void migrateExpiryColumn(pqxx::work &transaction, const std::string &column) {
    if (columnType(transaction, column) == "timestamp with time zone") return;
    const std::string value = "BTRIM(" + column + "::text)";
    transaction.exec("ALTER TABLE subscribers ALTER COLUMN " + column +
                     " TYPE TIMESTAMPTZ USING CASE "
                     "WHEN " + column + " IS NULL OR " + value + " IN ('', 'Inactive') THEN NULL "
                     "WHEN " + value + " ~ '^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01]) ([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]$' "
                     "AND TO_CHAR(TO_DATE(LEFT(" + value + ", 10), 'YYYY-MM-DD'), 'YYYY-MM-DD') = LEFT(" + value + ", 10) "
                     "THEN " + value + "::TIMESTAMPTZ ELSE NULL END");
}

void migrateIptvCount(pqxx::work &transaction) {
    const std::string type = columnType(transaction, "iptv_count");
    if (type == "integer") return;
    transaction.exec("ALTER TABLE subscribers ALTER COLUMN iptv_count TYPE INTEGER USING "
                     "CASE WHEN BTRIM(iptv_count::text) ~ '^[0-9]+$' "
                     "THEN LEAST(BTRIM(iptv_count::text)::INTEGER, 10) ELSE 0 END");
}
}

std::unique_ptr<pqxx::connection> DatabaseManager::conn;

std::map<std::string, std::string> DatabaseManager::loadEnv() {
    std::map<std::string, std::string> env;
    std::ifstream file;
    for (const char *path : {".env", "../.env"}) {
        file.open(path);
        if (file.is_open()) break;
        file.clear();
    }
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line.front() == '#') continue;
            const size_t position = line.find('=');
            if (position != std::string::npos) {
                env[trim(line.substr(0, position))] = trim(line.substr(position + 1));
            }
        }
    }

    for (const char *key : {"DB_NAME", "DB_USER", "DB_PASS", "DB_HOST", "DB_PORT"}) {
        if (const char *value = std::getenv(key); value && *value) env[key] = value;
    }
    return env;
}

pqxx::connection *DatabaseManager::getConnection() {
    if (!conn || !conn->is_open()) {
        const auto env = loadEnv();
        constexpr std::array<const char *, 4> required = {"DB_NAME", "DB_USER", "DB_PASS", "DB_HOST"};
        for (const char *key : required) {
            const auto value = env.find(key);
            if (value == env.end() || value->second.empty()) {
                throw std::runtime_error(std::string("Missing ") + key + " in .env. Copy .env.example and configure PostgreSQL first.");
            }
        }

        std::string connectionString = "dbname=" + connectionValue(env.at("DB_NAME")) +
                                       " user=" + connectionValue(env.at("DB_USER")) +
                                       " password=" + connectionValue(env.at("DB_PASS")) +
                                       " host=" + connectionValue(env.at("DB_HOST"));
        const auto port = env.find("DB_PORT");
        if (port != env.end() && !port->second.empty()) {
            connectionString += " port=" + connectionValue(port->second);
        }
        conn = std::make_unique<pqxx::connection>(connectionString);
    }
    return conn.get();
}

bool DatabaseManager::isConnected() {
    try {
        return getConnection()->is_open();
    } catch (...) {
        return false;
    }
}

void DatabaseManager::initTables() {
    try {
        pqxx::work transaction(*getConnection());

        transaction.exec("CREATE TABLE IF NOT EXISTS blocks ("
                         "id SERIAL PRIMARY KEY, "
                         "prev_hash TEXT NOT NULL, "
                         "block_hash TEXT NOT NULL, "
                         "block_data TEXT NOT NULL, "
                         "timestamp BIGINT NOT NULL, "
                         "nonce INTEGER, "
                         "hash_version SMALLINT NOT NULL DEFAULT 1);");
        transaction.exec("ALTER TABLE blocks ADD COLUMN IF NOT EXISTS nonce INTEGER;");
        transaction.exec("ALTER TABLE blocks ADD COLUMN IF NOT EXISTS hash_version SMALLINT DEFAULT 1;");
        transaction.exec("UPDATE blocks SET hash_version = 1 WHERE hash_version IS NULL;");
        transaction.exec("ALTER TABLE blocks ALTER COLUMN hash_version SET DEFAULT 1;");
        transaction.exec("CREATE INDEX IF NOT EXISTS blocks_block_hash_idx ON blocks (block_hash);");

        transaction.exec("CREATE TABLE IF NOT EXISTS subscribers ("
                         "name TEXT PRIMARY KEY CHECK (BTRIM(name) <> ''), "
                         "balance NUMERIC(12, 2) NOT NULL DEFAULT 0 CHECK (balance >= 0), "
                         "internet_expiry TIMESTAMPTZ, "
                         "internet_speed TEXT NOT NULL DEFAULT '1' CHECK (internet_speed IN ('1', '2', '4', '6')), "
                         "iptv_expiry TIMESTAMPTZ, "
                         "iptv_count INTEGER NOT NULL DEFAULT 0 CHECK (iptv_count BETWEEN 0 AND 10), "
                         "phone_expiry TIMESTAMPTZ);");

        // Legacy releases used TEXT for timestamps and channel counts. Check
        // the catalog first so normal startup never rewrites these columns.
        migrateExpiryColumn(transaction, "internet_expiry");
        migrateExpiryColumn(transaction, "iptv_expiry");
        migrateExpiryColumn(transaction, "phone_expiry");
        migrateIptvCount(transaction);

        transaction.exec("UPDATE subscribers SET "
                         "balance = COALESCE(balance, 0), "
                         "internet_speed = CASE WHEN internet_speed IN ('1', '2', '4', '6') THEN internet_speed ELSE '1' END, "
                         "iptv_count = LEAST(GREATEST(COALESCE(iptv_count, 0), 0), 10);");
        transaction.exec("ALTER TABLE subscribers ALTER COLUMN balance SET DEFAULT 0;");
        transaction.exec("ALTER TABLE subscribers ALTER COLUMN internet_speed SET DEFAULT '1';");
        transaction.exec("ALTER TABLE subscribers ALTER COLUMN iptv_count SET DEFAULT 0;");

        transaction.commit();
        std::cout << "[MAGLUMAT BAZASY]: Ähli jedweller taýýar." << std::endl;
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Database setup failed: ") + e.what());
    }
}
