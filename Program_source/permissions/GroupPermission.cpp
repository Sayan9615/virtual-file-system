#include "GroupPermission.h"
#include "../exceptions/PermissionException.h"
#include <sstream>
#include <algorithm>

GroupPermission::GroupPermission(const std::string &groupName, bool canRead, bool canWrite)
:Permission(canRead,canWrite),m_groupName(groupName)
{
}

GroupPermission::GroupPermission(const GroupPermission &other)
:Permission(other),m_groupName(other.m_groupName),m_members(other.m_members)
{
}

GroupPermission::GroupPermission(GroupPermission &&other) noexcept
:Permission(std::move(other)),m_groupName(std::move(other.m_groupName)),m_members(std::move(other.m_members))
{
}

GroupPermission &GroupPermission::operator=(const GroupPermission &other)
{
    if(this!=&other)
    {
        Permission::operator=(other);
        this->m_groupName=other.m_groupName;
        this->m_members=other.m_members;
    }
    return *this;
}

GroupPermission &GroupPermission::operator=(GroupPermission &&other) noexcept
{
     if(this!=&other)
    {
        Permission::operator=(std::move(other));
        this->m_groupName=std::move(other.m_groupName);
        this->m_members=std::move(other.m_members);
    }
    return *this;
}

void GroupPermission::addMember(const std::string &username)
{
    if(!this->isMember(username))
        this->m_members.push_back(username);
}

void GroupPermission::removeMember(const std::string &username)
{
    auto it=std::find(this->m_members.begin(),this->m_members.end(),username);
    if(it==this->m_members.end())
        throw PermissionException(username+" nu este membru al grupului "+this->m_groupName);

    this->m_members.erase(it);
}

bool GroupPermission::isMember(const std::string &username) const
{
    return std::find(this->m_members.begin(),this->m_members.end(),username)!=this->m_members.end();
}

bool GroupPermission::check(const std::string &username, const std::string &operation) const
{
    if(!this->isMember(username))
        return false;
   if(operation=="read") 
        return this->m_canRead;
   if(operation=="write") 
        return this->m_canWrite;   
   
    throw PermissionException("Operatie necunoscuta: "+operation);
}

std::string GroupPermission::serialize() const
{
   std::ostringstream oss;
   oss<<Permission::serialize()<<"|"<<this->m_groupName<<"|";

    for(int i =0;i<(int)this->m_members.size();i++)
    {
        oss<<this->m_members[i];
        if(i<(int)this->m_members.size()-1)
            oss<<",";
    }
    return oss.str();
}

void GroupPermission::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while(std::getline(iss,token,'|'))
         tokens.push_back(token);
    
    if(tokens.size()<4)
        throw PermissionException("Date invalide OwnerPermission!");
    
    this->m_canRead=tokens[1]=="1";
    this->m_canWrite=tokens[2]=="1";    
    this->m_groupName=tokens[3];

    if(tokens.size()>4 && !tokens[4].empty())
    {
        std::istringstream memberStream(tokens[4]);
        std::string member;

        while(std::getline(memberStream,member,','))
         this->m_members.push_back(member);

    }
}
