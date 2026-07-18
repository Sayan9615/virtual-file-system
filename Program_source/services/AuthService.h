#pragma once
#include "../interfaces/IAuthService.h"
#include "Database.h"
#include "../logger/Logger.h"

class AuthService : public IAuthService {
public:
    AuthService(Database& db, Logger& logger);

    bool initializeDatabase() override;
    bool registerUser(const std::string& username, const std::string& password) override;
    bool login(const std::string& username, const std::string& password) override;
    void logout() override;

    std::vector<std::pair<int, std::string>> getAllUsers() override;
    bool isAdmin(const std::string& username) override;
    bool deleteUser(const std::string& username) override;

private:
    Database& db;
    iLogger& logger;
};
