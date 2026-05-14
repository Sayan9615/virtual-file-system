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
        CREATE TABLE IF NOT EXISTS groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL
        );
        CREATE TABLE IF NOT EXISTS user_groups (
            user_id INTEGER,
            group_id INTEGER,
            FOREIGN KEY(user_id) REFERENCES users(id),
            FOREIGN KEY(group_id) REFERENCES groups(id)
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

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::login(const std::string& username, const std::string& password) {
    const char* sql = "SELECT id FROM users WHERE username = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;


    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_ROW);
    if(sqlite3_step(stmt) == SQLITE_ROW){
        std::string password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        PasswordHasher::initialize_sodium();
        success = PasswordHasher::verifyPassword(password,password_hash);
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

std::vector<std::string> AuthService::getAllGroups() {
    std::vector<std::string> groups;
    const char* sql = "SELECT name FROM groups;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name) groups.push_back(std::string(name));
    }
    sqlite3_finalize(stmt);
    return groups;
}

std::vector<std::string> AuthService::getUsersInGroup(const std::string& groupName) {
    std::vector<std::string> users;
    const char* sql = "SELECT u.username FROM users u JOIN user_groups ug ON u.id = ug.user_id JOIN groups g ON ug.group_id = g.id WHERE g.name = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name) users.push_back(std::string(name));
    }
    sqlite3_finalize(stmt);
    return users;
}

bool AuthService::createGroup(const std::string& groupName) {
    const char* sql = "INSERT INTO groups (name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::addUserToGroup(const std::string& groupName, const std::string& username) {
    const char* sql = "INSERT INTO user_groups (user_id, group_id) SELECT u.id, g.id FROM users u, groups g WHERE u.username = ? AND g.name = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, groupName.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::removeUserFromGroup(const std::string& groupName, const std::string& username) {
    const char* sql = "DELETE FROM user_groups WHERE user_id = (SELECT id FROM users WHERE username = ?) AND group_id = (SELECT id FROM groups WHERE name = ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, groupName.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}