    #include "Database.h"
    
    #include <iostream>

    Database::Database() : db(nullptr) {}

    Database::~Database() {
        close();
    }

    bool Database::open(const std::string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            //Add except
            //Add event Log

            if (db != nullptr) {
                sqlite3_close(db);
                db = nullptr;
            }

            return false;
        }

        return true;
    }

    void Database::close() {
        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    bool Database::execute(const std::string& sql) {
        if (db == nullptr) {
            //Add exception
            //Add eventLog
            return false;
        }

        char* errMsg = nullptr;
        int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

        if (result != SQLITE_OK) {
            //Add exception
            //Add EventLog
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

    sqlite3* Database::getConnection() const {
        return db;
    }