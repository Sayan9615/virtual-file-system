#include "OthersPermission.h"
#include "../exceptions/PermissionException.h"
#include <sstream>

OthersPermission::OthersPermission(bool canRead, bool canWrite)
:Permission(canRead,canWrite)
{
}

OthersPermission::OthersPermission(const OthersPermission &other)
:Permission(other)
{
}

OthersPermission::OthersPermission(OthersPermission &&other) noexcept
:Permission(std::move(other))
{
}

OthersPermission &OthersPermission::operator=(const OthersPermission &other)
{
    if(this!=&other)
    {
        Permission::operator=(other);
    }
    return *this;
}

OthersPermission &OthersPermission::operator=(OthersPermission &&other) noexcept
{
    if(this!=&other)
    {
        Permission::operator=(std::move(other));
    }
    return *this;
}

bool OthersPermission::check(const std::string &username,const std::string &operation) const
{
   if(operation=="read") 
        return this->m_canRead;
   if(operation=="write") 
        return this->m_canWrite;   
   
    throw PermissionException("Operatie necunoscuta: "+operation);
}

std::string OthersPermission::serialize() const
{
    return Permission::serialize();
}

void OthersPermission::deserialize(const std::string &data)
{
      Permission::deserialize(data);
}
