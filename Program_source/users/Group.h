#pragma once
#include "../interfaces/ISerializable.h"
#include <string>
#include <vector>

class Group : public iSerializable {
    std::string m_name;
    std::vector<std::string> m_members;

public:
    Group() = default;
    explicit Group(const std::string& name);

    std::string getName() const { return m_name; }
    const std::vector<std::string>& getMembers() const { return m_members; }

    void addMember(const std::string& username);
    void removeMember(const std::string& username);
    bool isMember(const std::string& username) const;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;
};