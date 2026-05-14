#include "UserPermission.h"
#include <sstream>

UserPermission::UserPermission() : Permission(false,false){}

UserPermission::UserPermission(const UserPermission& other) 
: Permission(other), m_users(other.m_users){}

UserPermission& UserPermission::operator=(const UserPermission& other) {
    if (this != &other) {
        Permission::operator=(other);
        m_users = other.m_users;
    }
    return *this;
}

UserPermission& UserPermission::operator=(UserPermission&& other) noexcept {
    if (this != &other) {
        Permission::operator=(std::move(other));
        m_users = std::move(other.m_users);
    }
    return *this;
}

void UserPermission::addUser(const std::string& username, bool canRead, bool canWrite) {
    for (int i = 0; i < m_users.size();i++) {
        if (m_users[i].username == username) {
            m_users[i].can_read = canRead;
            m_users[i].can_write = canWrite;
            return;
        }
    }
    m_users.push_back({username, canRead, canWrite});
}

void UserPermission::removeUser(const std::string& username) {
    for (int i = 0; i < m_users.size(); i++) {
        if (m_users[i].username == username) {
            m_users.erase(m_users.begin() + i);
            return;
        }
    }
}   

bool UserPermission::hasUser(const std::string& username) const{
    for(int i = 0; i < m_users.size();i++){
        if(m_users[i].username == username) return true;
    }
    return false;
}

const std::vector<UserAccess>& UserPermission::getUsers() const {
    return m_users;
}

bool UserPermission::check(const std::string& username, const std::string& operation) const {
    for (int i = 0; i < m_users.size();i++) {
        if (m_users[i].username == username) {
            if (operation == "read")  return m_users[i].can_read;
            if (operation == "write") return m_users[i].can_write;
        }
    }
    return false;
}

std::string UserPermission::serialize() const {
    std::ostringstream ss;
    for (size_t i = 0; i < m_users.size(); ++i) {
        if (i > 0) ss << ";";
        ss << m_users[i].username << ":"
           << (m_users[i].can_read  ? "1" : "0") << ":"
           << (m_users[i].can_write ? "1" : "0");
    }
    return ss.str();
}

void UserPermission::deserialize(const std::string& data) {
    m_users.clear();
    std::istringstream ss(data);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;
        std::istringstream ts(token);
        std::string username, read, write;
        std::getline(ts, username, ':');
        std::getline(ts, read, ':');
        std::getline(ts, write, ':');
        m_users.push_back({username, read == "1", write == "1"});
    }
}