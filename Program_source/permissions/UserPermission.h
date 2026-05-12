#pragma once
#include "Permission.h"
#include <string>
#include <vector>

struct UserAccess{
    std::string username;
    bool can_read;
    bool can_write;
};

class UserPermission : public Permission
{
    protected:
        std::vector<UserAccess> m_users;

    public:
        UserPermission();
        UserPermission(const UserPermission& other);
        UserPermission(UserPermission&& other) noexcept;

        ~UserPermission() = default;

        UserPermission& operator=(const UserPermission& other);
        UserPermission& operator=(UserPermission&& other) noexcept;


        void addUser(const std::string& username, bool can_read, bool can_write);
        void removeUser(const std::string& username);
        bool hasUser(const std::string& username) const;
        const std::vector<UserAccess>& getUsers() const;

        bool check(const std::string& username, const std::string& operation) const override;
        std::string getType() const override {return "USER";}

        std::string serialize() const override;
        void deserialize(const std::string& data) override;

};