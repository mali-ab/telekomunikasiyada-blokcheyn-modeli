#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <pqxx/pqxx>
#include <map>
#include <string>
#include <fstream>

class DatabaseManager {
public:
    static pqxx::connection* getConnection();
    static std::map<std::string, std::string> loadEnv();
    static void initTables();
private:
    static pqxx::connection* conn;
};

#endif