#pragma once
#include <string>
#include <vector>
#include "Database.h"
#include "User.h"
#include "EventLog.h"
#include "../interfaces/ILogger.h"
#include "IAuthService.h"

class AuthService : public IAuthService {
public:
    AuthService(Database& database, iLogger& logger);

    bool initializeDatabase();

    virtual bool registerUser(const std::string& username, const std::string& password) override;
    virtual bool login(const std::string& username, const std::string& password) override;
    virtual void logout() override;

    bool isAuthenticated() const;
    User getCurrentUser() const;

    virtual std::vector<std::pair<int, std::string>> getAllUsers() const override;

    virtual bool createGroup(const std::string& groupName) override;
    virtual bool addUserToGroup(const std::string& groupName, const std::string& username) override;
    virtual bool removeUserFromGroup(const std::string& groupName, const std::string& username) override;
    virtual std::vector<std::string> getAllGroups() const override;
    virtual std::vector<std::string> getUsersInGroup(const std::string& groupName) const override;

private:
    Database& database;
    iLogger& logger;
    User currentUser;

    bool userExists(const std::string& username);
    bool getUserByUsername(const std::string& username, User& user, std::string& passwordHash);

    bool isValidUsername(const std::string& username);
    bool isValidPassword(const std::string& password);
};