#pragma once
#include "ISerializable.h"
#include <string>


class BaseUser : public iSerializable
{
    protected:
        int id;
        std::string username;
        std::string passwordHash;
    public:
        BaseUser(std::string username, std::string passwordHash) :
        username(username), passwordHash(passwordHash) {};
        int getID();
};
