#pragma once
#include "Folder.h"
#include "../interfaces/iShareable.h"
#include <vector>
#include <string>

class SharedFolder : public Folder, public iShareable
{
    private:
        std::vector<std::string> m_shareWith;
        bool m_isPublic;
    
    public:
        SharedFolder(const std::string& name ,const std::string& ownerUser,const std::string ownerGroup,Folder* parent=nullptr,bool isPublic=false);
        SharedFolder(const SharedFolder& other);
        SharedFolder(SharedFolder && other)noexcept;

        //operator de atribuire
        SharedFolder& operator=(const SharedFolder& other);
        SharedFolder& operator=(SharedFolder&& other) noexcept;

        //getter
        bool isPublic()const {return this->m_isPublic;}

        //setter
        void setPublic(bool isPublic){this->m_isPublic=isPublic;}

        void display() const override;
        std::string getIcon() const override;

        void share(const std::string& username)override;
        void revokeAccess(const std::string& username) override;
        std::vector<std::string> getSharedWith() const override;

        bool hasAccess(const std::string& username)const;

        std::string serialize()const override;
        void deserialize(const std::string& data)override;

        bool operator==(const SharedFolder& other)const;
        friend std::ostream& operator<<(std::ostream& os ,const SharedFolder& folder);      

};
