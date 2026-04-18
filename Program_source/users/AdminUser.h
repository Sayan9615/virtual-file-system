#pragma once
#include "BaseUser.h"

class AdminUser : public BaseUser 
{
    public:
        AdminUser() : BaseUser() {};
        AdminUser(int id, std::string username) : BaseUser(id,username){};
};