#pragma once
#include "BaseUser.h"

class AdminUser : public BaseUser 
{
    public:
        AdminUser(std::string name,std::string passwordhash) : BaseUser(name,passwordhash){};
};