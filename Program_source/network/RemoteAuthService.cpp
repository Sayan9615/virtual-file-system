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
    return username == "admin"; // Verificare simpla locala pt admin
}

bool RemoteAuthService::deleteUser(const std::string& username) {
    auto parts = split(sendCmd("AUTH_DELETE_USER|" + username), '|');
    return !parts.empty() && parts[0] == "OK";
}

std::vector<std::string> RemoteAuthService::getAllGroups() {
    auto parts = split(sendCmd("AUTH_GET_ALL_GROUPS"), '|');
    std::vector<std::string> groups;
    if (parts.size() >= 1 && parts[0] == "OK") {
        for (size_t i = 1; i < parts.size(); ++i) {
            if (!parts[i].empty()) groups.push_back(parts[i]);
        }
    }
    return groups;
}

std::vector<std::string> RemoteAuthService::getUsersInGroup(const std::string& groupName) {
    auto parts = split(sendCmd("AUTH_GET_USERS_IN_GROUP|" + groupName), '|');
    std::vector<std::string> users;
    if (parts.size() >= 1 && parts[0] == "OK") {
        for (size_t i = 1; i < parts.size(); ++i) {
            if (!parts[i].empty()) users.push_back(parts[i]);
        }
    }
    return users;
}

bool RemoteAuthService::createGroup(const std::string& groupName) {
    auto parts = split(sendCmd("AUTH_CREATE_GROUP|" + groupName), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteAuthService::addUserToGroup(const std::string& groupName, const std::string& username) {
    auto parts = split(sendCmd("AUTH_ADD_USER_TO_GROUP|" + groupName + "|" + username), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteAuthService::removeUserFromGroup(const std::string& groupName, const std::string& username) {
    auto parts = split(sendCmd("AUTH_REMOVE_USER_FROM_GROUP|" + groupName + "|" + username), '|');
    return !parts.empty() && parts[0] == "OK";
}