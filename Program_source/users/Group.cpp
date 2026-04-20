#include "Group.h"
#include <algorithm>
#include <sstream>

Group::Group(const std::string& name) : m_name(name) {}

void Group::addMember(const std::string& username) {
    if (!isMember(username))
        m_members.push_back(username);
}

void Group::removeMember(const std::string& username) {
    m_members.erase(
        std::remove(m_members.begin(), m_members.end(), username),
        m_members.end()
    );
}

bool Group::isMember(const std::string& username) const {
    return std::find(m_members.begin(), m_members.end(), username) != m_members.end();
}

// Format: "groupName:user1,user2,user3"
std::string Group::serialize() const {
    std::ostringstream oss;
    oss << m_name << ":";
    for (size_t i = 0; i < m_members.size(); ++i) {
        if (i > 0) oss << ",";
        oss << m_members[i];
    }
    return oss.str();
}

void Group::deserialize(const std::string& data) {
    auto sep = data.find(':');
    m_name = data.substr(0, sep);
    if (sep == std::string::npos || sep + 1 >= data.size()) return;

    std::istringstream ss(data.substr(sep + 1));
    std::string member;
    while (std::getline(ss, member, ','))
        if (!member.empty())
            m_members.push_back(member);
}
