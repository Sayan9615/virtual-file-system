#pragma once
#include "BaseUser.h"

class User : public BaseUser
{
    public:
        User() : BaseUser() {};
        User(int id,std::string username) : BaseUser(id,username){};
};