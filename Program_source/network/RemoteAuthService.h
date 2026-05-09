#pragma once
#include "../services/IAuthService.h"
#include "SocketClient.h"
#include <string>
#include <vector>

class RemoteAuthService : public IAuthService {
public:
    explicit RemoteAuthService(SocketClient& client);

    bool registerUser(const std::string& username, const std::string& password) override;
    bool login(const std::string& username, const std::string& password) override;
    void logout() override;
    std::vector<std::pair<int, std::string>> getAllUsers() const override;
    bool createGroup(const std::string& groupName) override;
    bool addUserToGroup(const std::string& groupName, const std::string& username) override;
    bool removeUserFromGroup(const std::string& groupName, const std::string& username) override;
    std::vector<std::string> getAllGroups() const override;
    std::vector<std::string> getUsersInGroup(const std::string& groupName) const override;

private:
    SocketClient& m_client;
    // send is non-const; const overrides use const_cast internally
    std::string sendCmd(const std::string& cmd) const;
    static std::vector<std::string> split(const std::string& s, char delim);
};
