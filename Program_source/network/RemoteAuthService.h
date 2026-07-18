#pragma once
#include "../interfaces/IAuthService.h"
#include "SocketClient.h"
#include <string>
#include <vector>

class RemoteAuthService : public IAuthService {
public:
    explicit RemoteAuthService(SocketClient& client);

    bool initializeDatabase() override;
    bool registerUser(const std::string& username, const std::string& password) override;
    bool login(const std::string& username, const std::string& password) override;
    void logout() override;

    std::vector<std::pair<int, std::string>> getAllUsers() override;
    bool isAdmin(const std::string& username) override;
    bool deleteUser(const std::string& username) override;

private:
    SocketClient& m_client;
    std::string sendCmd(const std::string& cmd);
    static std::vector<std::string> split(const std::string& s, char delim);
};
