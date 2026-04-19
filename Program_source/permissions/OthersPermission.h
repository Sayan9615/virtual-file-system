#pragma once 
#include "Permission.h"
#include <string>
#include <vector>

class OthersPermission : public Permission
{
    public:
     OthersPermission(bool canRead=false, bool canWrite=false);
     OthersPermission(const OthersPermission& other);
     OthersPermission(OthersPermission&& other)noexcept;

     OthersPermission& operator=(const OthersPermission& other);
     OthersPermission& operator=(OthersPermission&& other)noexcept;

     ~OthersPermission()=default;

     bool check(const std::string& username,const std::string& operation) const override;
     std::string getType()const override{return "OTHERS";}

     std::string serialize() const override;
     void deserialize(const std::string& data) override;

};