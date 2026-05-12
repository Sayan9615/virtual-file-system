#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../permissions/Permission.h"
#include "../permissions/OwnerPermission.h"
#include "../permissions/UserPermission.h"
#include "../permissions/OthersPermission.h"

class PermissionManager {
public:
    PermissionManager();

    void addPermission(std::shared_ptr<Permission> permission);
    void removePermission(const std::string& type);

    bool canRead(const std::string& username) const;
    bool canWrite(const std::string& username) const;
    bool check(const std::string& username, const std::string& operation) const;

    std::shared_ptr<OwnerPermission> getOwnerPermission() const;
    std::shared_ptr<UserPermission> getUserPermission() const;
    std::shared_ptr<OthersPermission> getOthersPermission() const;

    std::string serialize() const;
    void deserialize(const std::string& data);

private:
    std::vector<std::shared_ptr<Permission>> permissions;
};