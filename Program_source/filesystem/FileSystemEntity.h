#pragma once

#include "../interfaces/iSerializable.h"
#include "../interfaces/iDisplayable.h"
#include <string>
#include <memory>
#include <ctime>
#include <vector>


 class FileSystemEntity:public iSerializable, public iDisplayable
 {
    protected:
        std::string m_name;
        std::string m_ownerUser;
        std::string m_ownerGroup;
        std::time_t m_createdAt;
        std::time_t m_modifiedAt;

    public:
        FileSystemEntity(const std::string& name,const std::string& ownerUser,const std::string& ownerGroup);
        FileSystemEntity(const FileSystemEntity& other);
        FileSystemEntity(FileSystemEntity && other)noexcept;
        
        //operatori de atribuire
        FileSystemEntity& operator=(const FileSystemEntity& other);    
        FileSystemEntity& operator=(FileSystemEntity&& other)noexcept;

        virtual ~FileSystemEntity()=default;

        //getteri
        std::string getName() const {return m_name;}
        std::string getOwnerUser()const {return m_ownerUser;}
        std::string getOwnerGroup()const {return m_ownerGroup;}
        std::time_t getCreatedAt()const {return m_createdAt;}
        std::time_t getModifiedAt()const {return m_modifiedAt;}


        //setteri
        void setName(const std::string& name);
        void setOwnerUser(const std::string& user);
        void setOwnerGroup(const std::string& group);


        virtual std::size_t getSize()const =0;
        virtual bool isFolder() const =0;

        //iDisplayable
        virtual void display() const override;
        virtual std::string getIcon()const override;

        //iSerializable
        virtual std::string serialize()const override;
        virtual void deserialize(const std::string& data) override;

        //operatori
        bool operator==(const FileSystemEntity& other)const;
        bool operator!=(const FileSystemEntity& other)const;
        bool operator<(const FileSystemEntity& other)const; //pentru sortare dupa nume

        friend std::ostream& operator<<(std::ostream& os,const FileSystemEntity& entity);

    };