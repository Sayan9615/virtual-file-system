#pragma once
#include "BaseUser.h"

class User : public BaseUser
{
    public:
        User(std::string name,std::string passwordhash) : BaseUser(name,passwordhash){};
};