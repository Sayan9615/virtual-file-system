#include "RemoteAuthService.h"
#include <sstream>

RemoteAuthService::RemoteAuthService(SocketClient& client) : m_client(client) {}

std::string RemoteAuthService::sendCmd(const std::string& cmd) {
    return m_client.sendCommand(cmd);
}

std::vector<std::string> RemoteAuthService::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        result.push_back(token);
    }
    return result;
}

bool RemoteAuthService::initializeDatabase() { return true; }

bool RemoteAuthService::registerUser(const std::string& username, const std::string& password) {
    
    auto parts = split(sendCmd("AUTH_REGISTER|" + username + "|" + password), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteAuthService::login(const std::string& username, const std::string& password) {
    auto parts = split(sendCmd("AUTH_LOGIN|" + username + "|" + password), '|');
    return !parts.empty() && parts[0] == "OK";
}

void RemoteAuthService::logout() {
    sendCmd("AUTH_LOGOUT");
}

std::vector<std::pair<int, std::string>> RemoteAuthService::getAllUsers() {
    auto parts = split(sendCmd("AUTH_GET_ALL_USERS"), '|');
    std::vector<std::pair<int, std::string>> users;
    if (parts.size() >= 1 && parts[0] == "OK") {
        for (size_t i = 1; i + 1 < parts.size(); i += 2) {
            users.push_back({std::stoi(parts[i]), parts[i+1]});
        }
    }
    return users;
}

bool RemoteAuthService::isAdmin(const std::string& username) {
    return username == "admin";
}

bool RemoteAuthService::deleteUser(const std::string& username) {
    auto parts = split(sendCmd("AUTH_DELETE_USER|" + username), '|');
    return !parts.empty() && parts[0] == "OK";
}
