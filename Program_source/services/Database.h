#pragma once
#include <string>
#include <sqlite3.h>

class Database
{
    private:
    sqlite3* db;

    public:
    Database();
    ~Database();

    bool open(const std::string& dbName);
    void close();

    bool execute(const std::string& sql);

    sqlite3* getConnection() const;

};