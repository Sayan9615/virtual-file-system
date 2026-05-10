#include "RemoteAuthService.h"
#include <sstream>
#include <stdexcept>

RemoteAuthService::RemoteAuthService(SocketClient& client)
    : m_client(client) {}

std::string RemoteAuthService::sendCmd(const std::string& cmd) const {
    return const_cast<SocketClient&>(m_client).sendCommand(cmd);
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

bool RemoteAuthService::registerUser(const std::string& username, const std::string& password) {
    std::string resp = sendCmd("AUTH_REGISTER|" + username + "|" + password);
    auto parts = split(resp, '|');
    if (parts.empty()) return false;
    if (parts[0] == "OK") return true;
    if (parts.size() > 1) throw std::runtime_error(parts[1]);
    return false;
}

bool RemoteAuthService::login(const std::string& username, const std::string& password) {
    std::string resp = sendCmd("AUTH_LOGIN|" + username + "|" + password);
    auto parts = split(resp, '|');
    return !parts.empty() && parts[0] == "OK";
}

void RemoteAuthService::logout() {
    sendCmd("AUTH_LOGOUT");
}

std::vector<std::pair<int, std::string>> RemoteAuthService::getAllUsers() const {
    std::string resp = sendCmd("AUTH_GET_ALL_USERS");
    auto parts = split(resp, '|');
    std::vector<std::pair<int, std::string>> result;
    if (parts.empty() || parts[0] != "OK") return result;
    // parts: OK | id1 | name1 | id2 | name2 | ...
    for (size_t i = 1; i + 1 < parts.size(); i += 2) {
        int id = std::stoi(parts[i]);
        result.push_back({id, parts[i + 1]});
    }
    return result;
}

bool RemoteAuthService::createGroup(const std::string& groupName) {
    std::string resp = sendCmd("AUTH_CREATE_GROUP|" + groupName);
    auto parts = split(resp, '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteAuthService::addUserToGroup(const std::string& groupName, const std::string& username) {
    std::string resp = sendCmd("AUTH_ADD_USER_TO_GROUP|" + groupName + "|" + username);
    auto parts = split(resp, '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteAuthService::removeUserFromGroup(const std::string& groupName, const std::string& username) {
    std::string resp = sendCmd("AUTH_REMOVE_USER_FROM_GROUP|" + groupName + "|" + username);
    auto parts = split(resp, '|');
    return !parts.empty() && parts[0] == "OK";
}

std::vector<std::string> RemoteAuthService::getAllGroups() const {
    std::string resp = sendCmd("AUTH_GET_ALL_GROUPS");
    auto parts = split(resp, '|');
    std::vector<std::string> result;
    if (parts.empty() || parts[0] != "OK") return result;
    for (size_t i = 1; i < parts.size(); ++i) {
        if (!parts[i].empty()) result.push_back(parts[i]);
    }
    return result;
}

std::vector<std::string> RemoteAuthService::getUsersInGroup(const std::string& groupName) const {
    std::string resp = sendCmd("AUTH_GET_USERS_IN_GROUP|" + groupName);
    auto parts = split(resp, '|');
    std::vector<std::string> result;
    if (parts.empty() || parts[0] != "OK") return result;
    for (size_t i = 1; i < parts.size(); ++i) {
        if (!parts[i].empty()) result.push_back(parts[i]);
    }
    return result;
}
