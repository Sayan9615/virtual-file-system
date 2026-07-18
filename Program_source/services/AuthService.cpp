#include "AuthService.h"
#include "../external/PasswordHasher.h"
#include <sqlite3.h>

AuthService::AuthService(Database& db, Logger& logger) : db(db), logger(logger) {}

bool AuthService::initializeDatabase() {
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL
        );
    )";
    if (db.execute(sql)) {
        // Defaadminult 
        registerUser("admin", "admin");
        return true;
    }
    return false;
}

bool AuthService::registerUser(const std::string& username, const std::string& password) {
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    PasswordHasher::initialize_sodium();
    std::string passwordHash = PasswordHasher::hashPassword(password);

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}
bool AuthService::login(const std::string& username, const std::string& password) {
    const char* sql = "SELECT password FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool success = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* storedRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string storedHash = storedRaw ? storedRaw : "";
        PasswordHasher::initialize_sodium();
        success = PasswordHasher::verifyPassword(password, storedHash);
        if (!success && storedHash == password)
            success = true;
    }

    sqlite3_finalize(stmt);
    return success;
}

void AuthService::logout() {
    // Logică pentru logout server-side
}

std::vector<std::pair<int, std::string>> AuthService::getAllUsers() {
    std::vector<std::pair<int, std::string>> users;
    const char* sql = "SELECT id, username FROM users;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name) users.push_back({id, std::string(name)});
    }
    sqlite3_finalize(stmt);
    return users;
}

bool AuthService::isAdmin(const std::string& username) {
    return username == "admin";
}

bool AuthService::deleteUser(const std::string& username) {
    if (username == "admin") return false; // Admin cannot be deleted
    const char* sql = "DELETE FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}
