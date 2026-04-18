#include "Database.h"
#include <iostream>

Database::Database() : db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open(const std::string& dbName) {
    if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Eroare la deschiderea bazei de date: "
                  << sqlite3_errmsg(db) << '\n';

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
        std::cerr << "Baza de date nu este deschisa.\n";
        return false;
    }

    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::cerr << "Eroare SQL: " << errMsg << '\n';
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

sqlite3* Database::getConnection() const {
    return db;
}