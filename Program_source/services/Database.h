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

    
    
    std::string serialize() const override { return NULL;};

    void deserialize(const std:: string &data) override {};

    void exportTo(const std::string& path) const override {};

    std::string getFormat() const override { return NULL;};

};