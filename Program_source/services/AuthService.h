#pragma once
#include <string>
#include <vector>
#include "Database.h"
#include "User.h"
#include "EventLog.h"
#include "../interfaces/ILogger.h"

class AuthService {
public:
    AuthService(Database& database, iLogger& logger);

    bool initializeDatabase();

    bool registerUser(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void logout();

    bool isAuthenticated() const;
    User getCurrentUser() const;

    std::vector<std::pair<int, std::string>> getAllUsers() const;

    bool createGroup(const std::string& groupName);
    bool addUserToGroup(const std::string& groupName, const std::string& username);
    bool removeUserFromGroup(const std::string& groupName, const std::string& username); // FIX: ADAUGAT AICI
    std::vector<std::string> getAllGroups() const;
    std::vector<std::string> getUsersInGroup(const std::string& groupName) const;

private:
    Database& database;
    iLogger& logger;
    User currentUser;

    bool userExists(const std::string& username);
    bool getUserByUsername(const std::string& username, User& user, std::string& passwordHash);

    bool isValidUsername(const std::string& username);
    bool isValidPassword(const std::string& password);
};