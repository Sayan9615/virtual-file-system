#pragma once
#include <string>
#include <vector>

class IAuthService {
public:
    virtual ~IAuthService() = default;

    virtual bool initializeDatabase() = 0;
    virtual bool registerUser(const std::string& username,
                              const std::string& password) = 0;
    virtual bool login(const std::string& username,
                       const std::string& password) = 0;
    virtual void logout() = 0;

    virtual std::vector<std::pair<int, std::string>> getAllUsers() = 0;

    // ── Admin ─────────────────────────────────────────
    virtual bool isAdmin(const std::string& username) = 0;
    virtual bool deleteUser(const std::string& username) = 0;

    // ── Grupuri — pastrate pentru compatibilitate interna
    virtual std::vector<std::string> getAllGroups() = 0;
    virtual std::vector<std::string> getUsersInGroup(
        const std::string& groupName) = 0;
    virtual bool createGroup(const std::string& groupName) = 0;
    virtual bool addUserToGroup(const std::string& groupName,
                                const std::string& username) = 0;
    virtual bool removeUserFromGroup(const std::string& groupName,
                                     const std::string& username) = 0;
};