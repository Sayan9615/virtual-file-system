#pragma once
#include "../interfaces/ISerializable.h"
#include <string>

class Permission : public iSerializable
{
    protected:
        bool m_canRead;
        bool m_canWrite;
    
    public:
        Permission(bool canRead,bool canWrite);
        Permission(const Permission &other);
        Permission(Permission && other)noexcept;
        
        //operatori de atribuire
        Permission& operator=(const Permission& other);
        Permission& operator=(Permission && other) noexcept;

        virtual ~Permission()=default;

        bool canRead()const {return this->m_canRead;}
        bool canWrite()const {return this->m_canWrite;}

        
        void setCanRead(bool value){this->m_canRead=value;}
        void setCanWrite(bool value){this->m_canWrite=value;}

        virtual bool check(const std::string& username,const std::string& operation)const=0;

        virtual std::string getType()const=0;

        std::string serialize() const override;
        void deserialize(const std::string& data) override;

        bool operator==(const Permission& other)const;
        bool operator!=(const Permission& other)const;
        friend std::ostream& operator<<(std::ostream&os ,const Permission & perm);

};