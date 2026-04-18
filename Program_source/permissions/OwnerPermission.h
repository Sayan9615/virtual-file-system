#pragma once
#include "Permission.h"

class OwnerPermission : public Permission
{
    private:
        std::string m_ownerUsername;

    public:
        OwnerPermission(const std::string& ownerUsername,bool canRead=true,bool canWrite=true);
        OwnerPermission(const OwnerPermission &other);
        OwnerPermission(OwnerPermission&& other) noexcept;
        
        OwnerPermission& operator=(const OwnerPermission& other);
        OwnerPermission& operator=(OwnerPermission&& other)noexcept;

        ~OwnerPermission()=default;

        std::string getOwnerUsername()const {return this->m_ownerUsername;}

        bool check(const std::string& username,const std::string& operation)const override;
        std::string getType()const override {return "OWNER";}

        std::string serialize()const override;
        void deserialize(const std::string& data)override;
};