#include "PermissionManager.h"
#include <sstream>

PermissionManager::PermissionManager() {
    permissions.push_back(std::make_shared<OthersPermission>(false, false));
}

void PermissionManager::addPermission(std::shared_ptr<Permission> permission) {
    // Daca exista deja un permisiune de acelasi tip, o inlocuieste
    removePermission(permission->getType());
    permissions.push_back(permission);
}

void PermissionManager::removePermission(const std::string& type) {
    for (int i = static_cast<int>(permissions.size()) - 1; i >= 0; --i) {
        if (permissions[i]->getType() == type) {
            permissions.erase(permissions.begin() + i);
        }
    }
}

bool PermissionManager::canRead(const std::string& username) const {
    return check(username, "read");
}

bool PermissionManager::canWrite(const std::string& username) const {
    return check(username, "write");
}

bool PermissionManager::check(const std::string& username, const std::string& operation) const {
    for (const auto& perm : permissions) {
        if (perm->getType() == "OWNER") {
            auto owner = std::dynamic_pointer_cast<OwnerPermission>(perm);
            if (owner && owner->check(username, operation)) return true;
        }
        if (perm->getType() == "USER") {
            auto user = std::dynamic_pointer_cast<UserPermission>(perm);
            if (user && user->check(username, operation)) return true;
        }
        if (perm->getType() == "OTHERS") {
            if (perm->check(username, operation)) return true;
        }
    }
    return false;
}

std::shared_ptr<OwnerPermission> PermissionManager::getOwnerPermission() const {
    for (const auto& perm : permissions) {
        if (perm->getType() == "OWNER")
            return std::dynamic_pointer_cast<OwnerPermission>(perm);
    }
    return nullptr;
}

std::shared_ptr<UserPermission> PermissionManager::getUserPermission() const {
    for (const auto& perm : permissions) {
        if (perm->getType() == "USER")
            return std::dynamic_pointer_cast<UserPermission>(perm);
    }
    return nullptr;
}

std::shared_ptr<OthersPermission> PermissionManager::getOthersPermission() const {
    for (const auto& perm : permissions) {
        if (perm->getType() == "OTHERS")
            return std::dynamic_pointer_cast<OthersPermission>(perm);
    }
    return nullptr;
}

std::string PermissionManager::serialize() const {
    std::ostringstream oss;
    for (const auto& perm : permissions) {
        oss << perm->getType() << ":" << perm->serialize() << ";";
    }
    return oss.str();
}

void PermissionManager::deserialize(const std::string& data) {
    // de implementat daca e nevoie
}
