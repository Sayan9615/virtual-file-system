#pragma once
#include <string>
#include <vector>

class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual bool registerUser(const std::string& username, const std::string& password) = 0;
    virtual bool login(const std::string& username, const std::string& password) = 0;
    virtual void logout() = 0;
    virtual std::vector<std::pair<int, std::string>> getAllUsers() const = 0;
    virtual bool createGroup(const std::string& groupName) = 0;
    virtual bool addUserToGroup(const std::string& groupName, const std::string& username) = 0;
    virtual bool removeUserFromGroup(const std::string& groupName, const std::string& username) = 0;
    virtual std::vector<std::string> getAllGroups() const = 0;
    virtual std::vector<std::string> getUsersInGroup(const std::string& groupName) const = 0;
};
