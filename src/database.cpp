#include "database.h"

std::unique_ptr<pqxx::connection> DatabaseManager::conn;

std::map<std::string, std::string> DatabaseManager::loadEnv() {
    std::map<std::string, std::string> env;
    std::ifstream file;
    for (const char *path : {".env", "../.env"}) {
        file.open(path);
        if (file.is_open()) break;
    }
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            env[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }
    return env;
}

pqxx::connection* DatabaseManager::getConnection() {
    if (!conn || !conn->is_open()) {
        auto env = loadEnv();
        std::string conn_str = "dbname=" + env["DB_NAME"] + " user=" + env["DB_USER"] +
                               " password=" + env["DB_PASS"] + " host=" + env["DB_HOST"];
        if (env.count("DB_PORT") && !env["DB_PORT"].empty()) {
            conn_str += " port=" + env["DB_PORT"];
        }
        conn = std::make_unique<pqxx::connection>(conn_str);
    }
    return conn.get();
}

bool DatabaseManager::isConnected() {
    try { return getConnection()->is_open(); }
    catch (...) { return false; }
}

void DatabaseManager::initTables() {
    try {
        pqxx::connection* C = getConnection();
        pqxx::work W(*C);

        W.exec("CREATE TABLE IF NOT EXISTS blocks ("
                "id SERIAL PRIMARY KEY, "
                "prev_hash TEXT, "
                "block_hash TEXT, "
                "block_data TEXT, "
                "timestamp BIGINT);");

        W.exec("CREATE TABLE IF NOT EXISTS subscribers ("
                "name TEXT PRIMARY KEY, "
                "balance NUMERIC(10, 2), "
                "internet_expiry TIMESTAMPTZ, "
                "internet_speed TEXT, "
                "iptv_expiry TIMESTAMPTZ, "
                "iptv_count INTEGER, "
                "phone_expiry TIMESTAMPTZ);");
        W.exec("ALTER TABLE subscribers ALTER COLUMN internet_expiry TYPE TIMESTAMPTZ USING NULLIF(internet_expiry::text, 'Inactive')::TIMESTAMPTZ");
        W.exec("ALTER TABLE subscribers ALTER COLUMN iptv_expiry TYPE TIMESTAMPTZ USING NULLIF(iptv_expiry::text, 'Inactive')::TIMESTAMPTZ");
        W.exec("ALTER TABLE subscribers ALTER COLUMN phone_expiry TYPE TIMESTAMPTZ USING NULLIF(phone_expiry::text, 'Inactive')::TIMESTAMPTZ");
        W.exec("ALTER TABLE subscribers ALTER COLUMN iptv_count TYPE INTEGER USING NULLIF(iptv_count::text, '')::INTEGER");

        W.commit();
        std::cout << "[MAGLUMAT BAZASY]: Ähli jedweller taýýar." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[BAŞLANGYÇ ÝALŇYŞLYGY]: " << e.what() << std::endl;
    }
}
