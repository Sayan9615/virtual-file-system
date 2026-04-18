#pragma once
#include "ISerializable.h"
#include "IExportable.h"

#include <string>
#include <sqlite3.h>

class Database : public iSerializable, public iExportable
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