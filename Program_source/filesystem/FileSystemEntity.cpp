#include "FileSystemEntity.h"
#include <iostream>
#include <sstream>
#include "../exceptions/FileSystemException.h"


FileSystemEntity::FileSystemEntity(const std::string &name, const std::string &ownerUser, const std::string &ownerGroup)
:m_name(name),m_ownerUser(ownerUser),m_ownerGroup(ownerGroup),m_createdAt(std::time(nullptr)),m_modifiedAt(std::time(nullptr))
{
    if(name.empty())
        throw FileSystemException("Numele entitatii nu poate gol!");
}

FileSystemEntity::FileSystemEntity(const FileSystemEntity &other)
:m_name(other.m_name),m_ownerUser(other.m_ownerUser),m_ownerGroup(other.m_ownerGroup),m_createdAt(other.m_createdAt),m_modifiedAt(other.m_modifiedAt)
{
}

FileSystemEntity::FileSystemEntity(FileSystemEntity &&other) noexcept
:m_name(std::move(other.m_name)),m_ownerUser(std::move(other.m_ownerUser)),m_ownerGroup(std::move(other.m_ownerGroup)),m_createdAt(std::move(other.m_createdAt)),m_modifiedAt(std::move(other.m_modifiedAt))
{
}

FileSystemEntity &FileSystemEntity::operator=(const FileSystemEntity &other)
{
    if(this!=&other)
    {
        this->m_name=other.m_name;
        this->m_ownerUser=other.m_ownerUser;
        this->m_ownerGroup=other.m_ownerGroup;
        this->m_createdAt=other.m_createdAt;
        this->m_modifiedAt=other.m_modifiedAt;

    }

    return *this;
}

FileSystemEntity &FileSystemEntity::operator=( FileSystemEntity &&other) noexcept
{
    if(this!=&other)
    {
        this->m_name=std::move(other.m_name);
        this->m_ownerUser=std::move(other.m_ownerUser);
        this->m_ownerGroup=std::move(other.m_ownerGroup);
        this->m_createdAt=other.m_createdAt;
        this->m_modifiedAt=other.m_modifiedAt;

    }

    return *this;
}

void FileSystemEntity::setName(const std::string &name)
{
    if(name.empty())
        throw FileSystemException("Numele nu poate fi gol!");

    this->m_name=name;
    this->m_modifiedAt=std::time(nullptr);    
}

void FileSystemEntity::setOwnerUser(const std::string &user)
{
    this->m_ownerUser=user;
}

void FileSystemEntity::setOwnerGroup(const std::string &group)
{
    this->m_ownerGroup=group;
}

void FileSystemEntity::display() const
{
    std::cout<<getIcon()<<" "<<this->m_name<<" | owner: "<<this->m_ownerUser<<" |group: "<<this->m_ownerGroup<<" | size: "<<this->getSize()<<" bytes"<<std::endl;
}

std::string FileSystemEntity::getIcon() const
{
    return this->isFolder()?"[DIR]":"[FILE]";
}

std::string FileSystemEntity::serialize() const
{
    std::ostringstream oss;
    oss<<this->m_name<<"|"<<this->m_ownerUser<<"|"<<this->m_ownerGroup<<"|"<<this->m_createdAt<<"|"<<this->m_modifiedAt;
    return oss.str();
}

void FileSystemEntity::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while(std::getline(iss,token,'|'))
    {
        tokens.push_back(token);
    }

    if(tokens.size()<5)
        throw FileSystemException("Date invalide pentru deserializare entitate!");

    this->m_name=tokens[0];
    this->m_ownerUser=tokens[1];
    this->m_ownerGroup=tokens[2];
    this->m_createdAt=std::stoll(tokens[3]);
    this->m_modifiedAt=std::stoll(tokens[4]);
}

bool FileSystemEntity::operator==(const FileSystemEntity &other) const
{
    return this->m_name == other.m_name && this->m_ownerGroup==other.m_ownerGroup;
}

bool FileSystemEntity::operator!=(const FileSystemEntity &other) const
{
    return !(*this==other);
}

bool FileSystemEntity::operator<(const FileSystemEntity &other) const
{
    return this->m_name<other.m_name;
}

std::ostream &operator<<(std::ostream &os, const FileSystemEntity &entity)
{
    os<< entity.getIcon()<<" "<<entity.m_name<<" ("<<entity.m_ownerUser<<")";
    return os;
}
