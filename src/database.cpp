#include "database.h"

pqxx::connection* DatabaseManager::conn = nullptr;

std::map<std::string, std::string> DatabaseManager::loadEnv() {
    std::map<std::string, std::string> env;
    std::ifstream file(".env");
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
    if (conn == nullptr) {
        auto env = loadEnv();
        std::string conn_str = "dbname=" + env["DB_NAME"] + " user=" + env["DB_USER"] +
                               " password=" + env["DB_PASS"] + " host=" + env["DB_HOST"];
        conn = new pqxx::connection(conn_str);
    }
    return conn;
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
                "internet_expiry TEXT, "
                "internet_speed TEXT, "
                "iptv_expiry TEXT, "
                "iptv_count TEXT, "
                "phone_expiry TEXT);");

        W.commit();
        std::cout << "[DATABASE]: All tables ready." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[INIT ERROR]: " << e.what() << std::endl;
    }
}