#include "Permission.h"
#include "../exceptions/PermissionException.h"
#include <iostream>
#include <sstream>
#include <vector>

Permission::Permission(bool canRead, bool canWrite)
:m_canRead(canRead),m_canWrite(canWrite)
{
}

Permission::Permission(const Permission &other)
:m_canRead(other.m_canRead),m_canWrite(other.m_canWrite)
{
}

Permission::Permission(Permission &&other) noexcept
:m_canRead(std::move(other.m_canRead)),m_canWrite(std::move(other.m_canWrite))
{
   other.m_canRead=false;
   other.m_canWrite=false; 
}

Permission &Permission::operator=(const Permission &other)
{
   if(this!=&other)
   {
        this->m_canRead=other.m_canRead;
        this->m_canWrite=other.m_canWrite;
   }
   return *this;
}

Permission &Permission::operator=(Permission &&other) noexcept
{
    
    if(this!=&other)
   {
        this->m_canRead=other.m_canRead;
        this->m_canWrite=other.m_canWrite;
        other.m_canRead=false;
        other.m_canWrite=false; 
   }
   return *this;
}

std::string Permission::serialize() const
{
   std::ostringstream oss;
   oss<<this->getType()<<"|"<<(this->m_canRead?"1":"0")<<"|"<<(this->m_canWrite?"1":"0");
   return oss.str();
}

void Permission::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string>tokens;

    while(std::getline(iss,token,'|'))
        tokens.push_back(token);

    if(tokens.size()<3)
        throw PermissionException("Date invalide pentru Permission!");
        
    this->m_canRead=tokens[1]=="1"; //mai simplu decat un if    
    this->m_canWrite=tokens[2]=="1";
}

bool Permission::operator==(const Permission &other) const
{
   return this->m_canRead==other.m_canRead && this->m_canWrite==other.m_canWrite;
}

bool Permission::operator!=(const Permission &other) const
{
    return !(*this==other);
}

std::ostream &operator<<(std::ostream &os, const Permission &perm)
{
   os<<perm.getType()<<" [r:"<<(perm.m_canRead?"y":"n")<<" w:"<<(perm.m_canWrite?"y":"n")<<"]";
   return os;
}
