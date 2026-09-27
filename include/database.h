#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <pqxx/pqxx>
#include <map>
#include <string>
#include <fstream>
#include <memory>

class DatabaseManager {
public:
    static pqxx::connection* getConnection();
    static std::map<std::string, std::string> loadEnv();
    static void initTables();
    static bool isConnected();
private:
    static std::unique_ptr<pqxx::connection> conn;
};

#endif
