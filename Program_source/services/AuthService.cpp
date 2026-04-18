#include "AuthService.h"
#include "PasswordHasher.h"
#include <sqlite3.h>
#include <iostream>

AuthService::AuthService(Database& database)
    : database(database), currentUser() {
}

bool AuthService::initializeDatabase() {
    std::string sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "password_hash TEXT NOT NULL"
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

    int result = sqlite3_step(stmt);

    if (result == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);

        const unsigned char* usernameText = sqlite3_column_text(stmt, 1);
        const unsigned char* passwordHashText = sqlite3_column_text(stmt, 2);

        std::string dbUsername = usernameText ? reinterpret_cast<const char*>(usernameText) : "";
        passwordHash = passwordHashText ? reinterpret_cast<const char*>(passwordHashText) : "";

        user = User(id, dbUsername);

        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

bool AuthService::registerUser(const std::string& username, const std::string& password) {
    std::cerr << "DEBUG: username='" << username << "' len=" << username.length() << "\n";
    std::cerr << "DEBUG: password='" << password << "' len=" << password.length() << "\n";

    if (!isValidUsername(username) || !isValidPassword(password)) {
        std::cerr << "DEBUG: validare esuata, parola sau username invalide\n";
        return false;
    }

    if (userExists(username)) {
        std::cerr << "DEBUG: userul exista deja\n";
        return false;
    }

    std::string passwordHash;
    try {
        passwordHash = PasswordHasher::hashPassword(password);
        std::cerr << "DEBUG: hash generat = " << passwordHash << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "DEBUG: exceptie la hash: " << ex.what() << '\n';
        return false;
    }

    const char* sql = "INSERT INTO users(username, password_hash) VALUES(?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(database.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "DEBUG: prepare esuat: " << sqlite3_errmsg(database.getConnection()) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    if (!success) {
        std::cerr << "DEBUG: insert esuat: " << sqlite3_errmsg(database.getConnection()) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

bool AuthService::login(const std::string& username, const std::string& password) {
    User user;
    std::string storedHash;

    if (!getUserByUsername(username, user, storedHash)) {
        return false;
    }

    if (!PasswordHasher::verifyPassword(password, storedHash)) {
        return false;
    }

    currentUser = user;
    return true;
}

void AuthService::logout() {
    currentUser.invalidate();
}

bool AuthService::isAuthenticated() const {
    return currentUser.isValid();
}

User AuthService::getCurrentUser() const {
    return currentUser;
}