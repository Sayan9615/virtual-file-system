#include "SharedFolder.h"
#include "../exceptions/FileSystemException.h"
#include "../exceptions/AccessDeniedException.h"
#include <iostream>
#include <sstream>
#include <algorithm>

SharedFolder::SharedFolder(const std::string &name, const std::string &ownerUser, const std::string ownerGroup, Folder *parent, bool isPublic)
:Folder(name,ownerUser,ownerGroup,parent),m_isPublic(isPublic)
{
}

SharedFolder::SharedFolder(const SharedFolder &other)
:Folder(other),m_shareWith(other.m_shareWith),m_isPublic(other.m_isPublic)
{
}

SharedFolder::SharedFolder(SharedFolder &&other) noexcept
:Folder(std::move(other)),m_shareWith(std::move(other.m_shareWith)),m_isPublic(other.m_isPublic)
{
}

SharedFolder &SharedFolder::operator=(const SharedFolder &other)
{
    if(this!=&other)
    {
        Folder::operator=(other);
        this->m_shareWith=other.m_shareWith;
        this->m_isPublic=other.m_isPublic;
    }
    return *this;
}

SharedFolder &SharedFolder::operator=(SharedFolder &&other) noexcept
{
     if(this!=&other)
    {
        Folder::operator=(std::move(other));
        this->m_shareWith=std::move(other.m_shareWith);
        this->m_isPublic=other.m_isPublic;
    }
    return *this;
}

void SharedFolder::display() const
{
    std::cout<<getIcon()<<" "<<this->m_name<<" | owner: "<<this->m_ownerUser<<" | elemente: "<<getChildCount()<<" | shared cu: "<<this->m_shareWith.size()<<" useri"<< (this->m_isPublic?" |PUBLIC":"PRIVAT")<<std::endl;
    
    for(const auto & child : this->m_children)
    {
        std::cout<<" ";
        child->display();
    }
}

std::string SharedFolder::getIcon() const
{
    return this->m_isPublic?"[DIR-PUBLIC]":"[DIR-SHARED]";
}

void SharedFolder::share(const std::string &username)
{
    if(username.empty())
        throw FileSystemException("Username invalid pentru partajare!");

     auto it=std::find(this->m_shareWith.begin(),this->m_shareWith.end(),username);
     if(it==this->m_shareWith.end())
     {
        this->m_shareWith.push_back(username);
        this->m_modifiedAt=std::time(nullptr);
     }   
}

void SharedFolder::revokeAccess(const std::string &username)
{
     auto it=std::find(this->m_shareWith.begin(),this->m_shareWith.end(),username);
     if(it==this->m_shareWith.end())
        throw AccessDeniedException(username+" nu are acces la folder!");
     
     this->m_shareWith.erase(it);
     this->m_modifiedAt=std::time(nullptr);   
}

std::vector<std::string> SharedFolder::getSharedWith() const
{
    return this->m_shareWith;
}

bool SharedFolder::hasAccess(const std::string &username) const
{
    if(this->m_isPublic) 
        return true;
    if(username==this->m_ownerUser)
        return true;
    
    auto it =std::find(this->m_shareWith.begin(),this->m_shareWith.end(),username);
    return it !=this->m_shareWith.end();  //returneaza rez evaluarii expresiei   
}

std::string SharedFolder::serialize() const
{
    std::ostringstream oss;
    oss<<Folder::serialize()<<"|"<<(this->m_isPublic?"1":"0")<<"|";

    for(int i =0 ;i<(int)this->m_shareWith.size();i++)
    {
       oss<<this->m_shareWith[i];
       if(i<(int)this->m_shareWith.size()-1)
        oss<<",";     
    }
    return oss.str();
}

void SharedFolder::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while(std::getline(iss,token,'|'))
        tokens.push_back(token);
    
    if(tokens.size()<7)
      throw FileSystemException("Date invalide pentru ShareFolder!");
    
    std::string baseData;
    for(int i = 0 ;i<6;i++)
    {
        baseData+=tokens[i];
        if(i<5)
            baseData+="|";
    }  
    Folder::deserialize(baseData);
    
    if(tokens[6]=="1")
     this->m_isPublic=token[6];

    if(tokens.size()>7 && !tokens[7].empty())
    {
        std::istringstream sharedStream(tokens[7]);
        std::string user;
        while(std::getline(sharedStream,user,','))
            this->m_shareWith.push_back(user);
    }

}

bool SharedFolder::operator==(const SharedFolder &other) const
{
    return this->m_name==other.m_name && this->m_ownerUser==other.m_ownerUser;
}

std::ostream &operator<<(std::ostream &os, const SharedFolder &folder)
{
    os<<folder.getIcon()<<" "<<folder.m_name<<" ("<<folder.getChildCount()<<" elemente, shared cu "<<folder.m_shareWith.size()<<"useri)";\
    return os;
}
