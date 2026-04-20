#include "GroupPermission.h"
#include "../exceptions/PermissionException.h"
#include <sstream>
#include <algorithm>

GroupPermission::GroupPermission(bool canRead, bool canWrite)
    : Permission(canRead, canWrite) {}

GroupPermission::GroupPermission(const GroupPermission& other)
    : Permission(other), m_groups(other.m_groups) {}

GroupPermission::GroupPermission(GroupPermission&& other) noexcept
    : Permission(std::move(other)), m_groups(std::move(other.m_groups)) {}

GroupPermission& GroupPermission::operator=(const GroupPermission& other) {
    if (this != &other) {
        Permission::operator=(other);
        m_groups = other.m_groups;
    }
    return *this;
}

GroupPermission& GroupPermission::operator=(GroupPermission&& other) noexcept {
    if (this != &other) {
        Permission::operator=(std::move(other));
        m_groups = std::move(other.m_groups);
    }
    return *this;
}

void GroupPermission::addGroup(const Group& group) {
    if (!hasGroup(group.getName()))
        m_groups.push_back(group);
}

void GroupPermission::removeGroup(const std::string& groupName) {
    m_groups.erase(
        std::remove_if(m_groups.begin(), m_groups.end(),
            [&groupName](const Group& g) { return g.getName() == groupName; }),
        m_groups.end()
    );
}

bool GroupPermission::hasGroup(const std::string& groupName) const {
    return std::any_of(m_groups.begin(), m_groups.end(),
        [&groupName](const Group& g) { return g.getName() == groupName; });
}

bool GroupPermission::isMember(const std::string& username) const {
    return std::any_of(m_groups.begin(), m_groups.end(),
        [&username](const Group& g) { return g.isMember(username); });
}

bool GroupPermission::check(const std::string& username, const std::string& operation) const {
    if (!isMember(username)) return false;
    if (operation == "read")  return m_canRead;
    if (operation == "write") return m_canWrite;
    throw PermissionException("Operatie necunoscuta: " + operation);
}

// Format: "groupName1:user1,user2|groupName2:user3"
std::string GroupPermission::serialize() const {
    std::ostringstream oss;
    oss << m_canRead << "|" << m_canWrite << "|";
    for (size_t i = 0; i < m_groups.size(); ++i) {
        if (i > 0) oss << ";";
        oss << m_groups[i].serialize();
    }
    return oss.str();
}

void GroupPermission::deserialize(const std::string& data) {
    // Format: "canRead|canWrite|group1:u1,u2;group2:u3"
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    if (tokens.size() < 3) return;

    m_canRead  = tokens[0] == "1";
    m_canWrite = tokens[1] == "1";

    std::istringstream groupStream(tokens[2]);
    std::string groupData;
    while (std::getline(groupStream, groupData, ';')) {
        if (groupData.empty()) continue;
        Group g;
        g.deserialize(groupData);
        m_groups.push_back(g);
    }
}
