#pragma once
#include "Permission.h"
#include <vector>

class GroupPermission : public Permission
{
    private:
        std::string m_groupName;
        std::vector<std::string> m_members;

    public:
        GroupPermission(const std::string& groupName,bool canRead=true, bool canWrite=false);
        GroupPermission(const GroupPermission& other);
        GroupPermission(GroupPermission&& other)noexcept;
        
        GroupPermission& operator=(const GroupPermission& other);    
        GroupPermission& operator=(GroupPermission&& other)noexcept;

        std::string getGroupName() const{return this->m_groupName;}
        std::vector<std::string> getMembers()const {return this->m_members;}

        void addMember(const std::string& username);
        void removeMember(const std::string& username);
        bool isMember(const std::string& username)const;

        bool check(const std::string& username,const std::string& operation) const override;
        std::string getType() const override{return "GROUP";}

        std::string serialize() const override;
        void deserialize(const std::string & data) override;
};  