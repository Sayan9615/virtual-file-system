#pragma once
#include <string>
#include "Database.h"
#include "User.h"

class AuthService {
private:
    Database& database;
    User currentUser;

public:
    AuthService(Database& database);

    bool initializeDatabase();

    bool registerUser(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void logout();

    bool isAuthenticated() const;
    User getCurrentUser() const;

private:
    bool userExists(const std::string& username);
    bool getUserByUsername(const std::string& username, User& user, std::string& passwordHash);

    bool isValidUsername(const std::string& username);
    bool isValidPassword(const std::string& password);
};