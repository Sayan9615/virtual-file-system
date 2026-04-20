#include "AuthService.h"
#include "PasswordHasher.h"
#include "EventLog.h"
#include <sqlite3.h>
#include <iostream>

AuthService::AuthService(Database& database, iLogger& logger)
    : database(database), logger(logger), currentUser() {
}

bool AuthService::initializeDatabase() {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "password_hash TEXT NOT NULL, "
        "role TEXT NOT NULL DEFAULT 'user'"
        ");";
    return database.execute(sql);
}

bool AuthService::isValidUsername(const std::string& username) {
    return !username.empty() && username.length() >= 3 && username.length() <= 30;
}

bool AuthService::isValidPassword(const std::string& password) {
    return !password.empty() && password.length() >= 6;
}

bool AuthService::userExists(const std::string& username) {
    const char* sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(database.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Eroare la prepare in userExists: "
                  << sqlite3_errmsg(database.getConnection()) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

bool AuthService::getUserByUsername(const std::string& username, User& user, std::string& passwordHash) {
    const char* sql = "SELECT id, username, password_hash FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(database.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Eroare la prepare in getUserByUsername: "
                  << sqlite3_errmsg(database.getConnection()) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* usernameText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* passwordHashText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        std::string dbUsername = usernameText ? usernameText : "";
        passwordHash = passwordHashText ? passwordHashText : "";

        user = User(id, dbUsername);
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

bool AuthService::registerUser(const std::string& username, const std::string& password) {
    if (!isValidUsername(username) || !isValidPassword(password)) {
        std::cerr << "Date invalide: user=" << username << " pass_len=" << password.length() << "\n";
        return false;
    }

    if (userExists(username)) {
        std::cerr << "Userul exista deja!\n";
        return false;
    }

    std::string passwordHash;
    try {
        passwordHash = PasswordHasher::hashPassword(password);
    } catch (const std::exception& ex) {
        std::cerr << "Eroare hash: " << ex.what() << "\n";
        return false;
    }

    const char* sql = "INSERT INTO users(username, password_hash, role) VALUES(?, ?, 'user');";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(database.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Eroare prepare INSERT: " << sqlite3_errmsg(database.getConnection()) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    int stepResult = sqlite3_step(stmt);
    if (stepResult != SQLITE_DONE) {
        std::cerr << "Eroare INSERT users: " << sqlite3_errmsg(database.getConnection()) << " (code " << stepResult << ")\n";
    }
    bool success = (stepResult == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success) {
        EventLog(EventType::REGISTER, username, "Utilizator inregistrat cu succes", logger);
    }

    return success;
}

bool AuthService::login(const std::string& username, const std::string& password) {
    User user;
    std::string storedHash;

    if (!getUserByUsername(username, user, storedHash)) {
        EventLog(EventType::LOGIN, username, "Login esuat: userul nu exista", logger);
        return false;
    }

    if (!PasswordHasher::verifyPassword(password, storedHash)) {
        EventLog(EventType::LOGIN, username, "Login esuat: parola incorecta", logger);
        return false;
    }

    currentUser = user;
    EventLog(EventType::LOGIN, username, "Login reusit", logger);
    return true;
}

void AuthService::logout() {
    EventLog(EventType::LOGOUT, currentUser.getUsername(), "Logout efectuat", logger);
    currentUser.invalidate();
}

bool AuthService::isAuthenticated() const {
    return currentUser.isValid();
}

User AuthService::getCurrentUser() const {
    return currentUser;
}

std::vector<std::pair<int, std::string>> AuthService::getAllUsers() const {
    const char* sql = "SELECT id, username FROM users ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    std::vector<std::pair<int, std::string>> users;

    if (sqlite3_prepare_v2(database.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return users;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        users.emplace_back(id, name ? name : "");
    }
    sqlite3_finalize(stmt);
    return users;
}