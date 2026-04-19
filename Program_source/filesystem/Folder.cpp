#include "Folder.h"
#include "../exceptions/FileSystemException.h"
#include "../exceptions/FileNotFoundException.h"
#include <iostream>
#include <sstream>
#include <numeric>

Folder::Folder(const std::string &name, const std::string &ownerUser, const std::string &ownerGroup, Folder *parent)
:FileSystemEntity(name,ownerUser,ownerGroup),m_index(3),m_parent(parent)
{
}

Folder::Folder(const Folder &other)
:FileSystemEntity(other),m_children(other.m_children),m_index(3),m_parent(other.m_parent)
{
    for(auto& child:this->m_children)
       this->m_index.insert(child->getName(),child); 
}

Folder::Folder(Folder &&other) noexcept
:FileSystemEntity(std::move(other)),m_children(std::move(other.m_children)),m_index(std::move(other.m_index)),m_parent(std::move(other.m_parent))
{
    other.m_parent=nullptr;
}

Folder &Folder::operator=(const Folder &other)
{
    if(this!=&other)
    {
        FileSystemEntity::operator=(other);
        this->m_children=other.m_children;
        this->m_parent=other.m_parent;

        this->m_index=BPlusTree<std::string ,std::shared_ptr<FileSystemEntity>>(3);
        for(auto &child:this->m_children)
            this->m_index.insert(child->getName(),child);
    }
    return *this;
}

Folder &Folder::operator=(Folder &&other) noexcept
{
    if(this!=&other)
    {
        FileSystemEntity::operator=(std::move(other));
        this->m_children=std::move(other.m_children);
        this->m_index=std::move(other.m_index);
        this->m_parent=other.m_parent;

        other.m_parent=nullptr;

    }
    return *this;
}

void Folder::addChild(std::shared_ptr<FileSystemEntity> entity)
{
    if(!entity)
        throw FileSystemException("Nu pot adauga ceva ce e null in folder!");
    
    if(hasChild(entity->getName()))
        throw FileSystemException("Exista deja un fisier/folder cu numele: "+entity->getName());
        
    this->m_children.push_back(entity);
    this->m_index.insert(entity->getName(),entity);
    this->m_modifiedAt=std::time(nullptr);

}

void Folder::removeChild(const std::string &name)
{
   for(int i = 0 ;i<(int)this->m_children.size();i++)
   {
        if(this->m_children[i]->getName()==name)
        {
            this->m_children.erase(this->m_children.begin()+i);

            this->m_index.remove(name);
            this->m_modifiedAt=std::time(nullptr);
            return;
        }
        
   }

   throw FileNotFoundException(this->getAbsolutePath()+"/"+name);

}

std::shared_ptr<FileSystemEntity> Folder::findChild(const std::string &name) const
{
    //B+Tree (log n)
    auto result=this->m_index.search(name);

    if(result.has_value())
        return result.value();
   throw FileNotFoundException(this->getAbsolutePath()+"/"+name);     
}

bool Folder::hasChild(const std::string &name) const
{
    return this->m_index.search(name).has_value();
}

std::string Folder::getAbsolutePath() const // recursiv e  mai usor
{
    if(this->m_parent==nullptr)
        return "/"+this->m_name;

   return this->m_parent->getAbsolutePath()+"/"+this->m_name;    
}

std::size_t Folder::getSize() const
{
    std::size_t total=0;

    for(const auto& child:this->m_children)
        total+=child->getSize();
    
   return total;     
}

void Folder::display() const
{
    std::cout<<getIcon()<<" "<<this->m_name<<" | owner: "<<this->m_ownerUser<<" | elemente: "<<this->m_children.size()<<" | size: "<<this->getSize()<<" bytes"<<std::endl;

    for(const auto& child : this->m_children)
    {
        std::cout<<" ";
        child->display();
    }

}

std::string Folder::getIcon() const
{
    return this->m_children.empty()?"[DIR-EMPTY]":"[DIR]";
}

std::vector<std::string> Folder::search(const std::string &text) const
{
    std::vector<std::string>results;

    for(const auto& child : this->m_children)
    {
        if(child->getName().find(text)!=std::string::npos)
            results.push_back(this->getAbsolutePath()+"/"+child->getName());
    
        //caut si in copil daca permite
        auto * searchable=dynamic_cast<iSearchable*>(child.get());
        if(searchable)
        {
            auto result2=searchable->search(text);
            results.insert(results.end(),result2.begin(),result2.end());
        }    
    }
    return results;

}

bool Folder::contains(const std::string &data) const
{
    return this->m_name.find(data)!= std::string::npos || hasChild(data);
}

std::string Folder::serialize() const
{
    //doar elementele folderului
    std::ostringstream oss;
    oss<<FileSystemEntity::serialize()<<"|"<<this->m_children.size();

    return oss.str();
}

void Folder::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string>tokens;

    while(std::getline(iss,token,'|'))
        tokens.push_back(token);
    
    if(tokens.size()<6)
        throw FileSystemException("Date invalide pentru folder!");
    
    std::string baseData;
    for(int i = 0 ;i<5;i++)
    {
        baseData+=tokens[i];
        if(i<4)
          baseData+="|";
    }
    FileSystemEntity::deserialize(baseData);
}

bool Folder::operator==(const Folder &other) const
{
    return this->m_name==other.m_name && this->m_ownerUser==other.m_ownerUser;
}

bool Folder::operator<(const Folder &other) const
{
    return this->m_name<other.m_name;
}

std::shared_ptr<FileSystemEntity> Folder::operator[](const std::string &name) const
{
    return findChild(name);
}

std::ostream &operator<<(std::ostream &os, const Folder &folder)
{
    os<<"[DIR]"<<folder.m_name<<" ("<<folder.m_children.size()<<" elemente)";

    return os;
}
