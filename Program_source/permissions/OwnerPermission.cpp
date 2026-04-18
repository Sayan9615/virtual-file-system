#include "OwnerPermission.h"
#include "../exceptions/PermissionException.h"
#include <sstream>
#include <vector>

OwnerPermission::OwnerPermission(const std::string &ownerUsername, bool canRead, bool canWrite)
:Permission(m_canRead,m_canWrite),m_ownerUsername(ownerUsername)
{
}

OwnerPermission::OwnerPermission(const OwnerPermission &other)
:Permission(other),m_ownerUsername(other.m_ownerUsername)
{
}

OwnerPermission::OwnerPermission(OwnerPermission &&other) noexcept
:Permission(std::move(other)),m_ownerUsername(std::move(other.m_ownerUsername))
{
}

OwnerPermission &OwnerPermission::operator=(const OwnerPermission &other)
{
    if(this!=&other)
    {
        Permission::operator=(other);
        this->m_ownerUsername=other.m_ownerUsername;
    }
    return *this;
}

OwnerPermission &OwnerPermission::operator=(OwnerPermission &&other) noexcept
{
    if(this!=&other)
    {
        Permission::operator=(std::move(other));
        this->m_ownerUsername=std::move(other.m_ownerUsername);
    }
    return *this;
}

bool OwnerPermission::check(const std::string &username, const std::string &operation) const
{
   if(username!=this->m_ownerUsername)
        return false;
   if(operation=="read") 
        return this->m_canRead;
   if(operation=="write") 
        return this->m_canWrite;   
   
    throw PermissionException("Operatie necunoscuta: "+operation);    
}

std::string OwnerPermission::serialize() const
{
    std::ostringstream oss;
    oss<<Permission::serialize()<<"|"<<this->m_ownerUsername;
    return oss.str();
}

void OwnerPermission::deserialize(const std::string &data)
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
    this->m_ownerUsername=tokens[3];
}
