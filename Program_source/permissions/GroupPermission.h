#pragma once
#include "Permission.h"
#include "../users/Group.h"
#include <vector>

class GroupPermission : public Permission {
    std::vector<Group> m_groups;

public:
    GroupPermission(bool canRead = true, bool canWrite = false);
    GroupPermission(const GroupPermission& other);
    GroupPermission(GroupPermission&& other) noexcept;

    GroupPermission& operator=(const GroupPermission& other);
    GroupPermission& operator=(GroupPermission&& other) noexcept;

    const std::vector<Group>& getGroups() const { return m_groups; }

    void addGroup(const Group& group);
    void removeGroup(const std::string& groupName);
    bool hasGroup(const std::string& groupName) const;
    bool isMember(const std::string& username) const;

    bool check(const std::string& username, const std::string& operation) const override;
    std::string getType() const override { return "GROUP"; }

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
};
