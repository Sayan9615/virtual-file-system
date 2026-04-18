#pragma once
#include "ISerializable.h"
#include <string>
#include <vector>
#include "BaseUser.h"

class Group : public iSerializable
{
   private:
        std::string nume;
        std::vector<BaseUser*> members;
    public:
        Group(std::string nume) : nume(nume){};
};
