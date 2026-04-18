#include "BinaryFile.h"
#include "../exceptions/FileSystemException.h"
#include "../exceptions/AccessDeniedException.h"
#include <iostream>
#include <sstream>
#include <algorithm>

BinaryFile::BinaryFile(const std::string &name, const std::string &ownerUser, const std::string &ownerGroup, const std::string &extension)
:File(name,ownerUser,ownerGroup,extension)
{
}

BinaryFile::BinaryFile(const BinaryFile &other)
:File(other),m_data(other.m_data),m_sharedWith(other.m_sharedWith)
{
}

BinaryFile::BinaryFile(BinaryFile &&other) noexcept
:File(std::move(other)),m_data(std::move(other.m_data)),m_sharedWith(std::move(other.m_sharedWith))
{
}

BinaryFile &BinaryFile::operator=(const BinaryFile &other)
{
    if(this!=&other)
    {
        File::operator=(other);
        this->m_data=other.m_data;
        this->m_sharedWith=other.m_sharedWith;
    }
    return *this;
}

BinaryFile &BinaryFile::operator=(BinaryFile &&other) noexcept
{
     if (this != &other) 
    {
        File::operator=(std::move(other));
        this->m_data    = std::move(other.m_data);
        m_sharedWith = std::move(other.m_sharedWith);
    }
    return *this;
}

std::string BinaryFile::read() const
{
    return "BinaryFile: " + m_name + m_extension +" (" + std::to_string(m_size) + " bytes)  continut binar, nu poate fi afisat";
}

void BinaryFile::write(const std::string &content)
{
    this->m_data.assign(content.begin(),content.end());
    this->m_size=this->m_data.size();
    this->m_modifiedAt=std::time(nullptr);
}

void BinaryFile::display() const
{
    std::cout << getIcon() << " " << m_name << m_extension << " | owner: "   << m_ownerUser<< " | size: "    << m_size << " bytes"<< " | shared: "  << m_sharedWith.size() << " useri"<< std::endl;
}

std::string BinaryFile::getIcon() const
{
    if(this->m_extension==".docx"||this->m_extension==".doc") return "[WORD]";
    if(this->m_extension==".pptx"||this->m_extension==".ppt") return "[PPT]";
    if(this->m_extension==".jpg"||this->m_extension==".png") return "[IMG]";
    if(this->m_extension==".pdf") return "[PDF]";

    return this->m_sharedWith.empty()?"[BIN]":"[BIN-SHARED]";
}

std::vector<std::string> BinaryFile::search(const std::string &text) const
{
   std::vector<std::string> results;
    if (contains(text)) 
        results.push_back(m_name + m_extension);
    
    return results;
}

bool BinaryFile::contains(const std::string &data) const
{
    return m_name.find(data) != std::string::npos;
}

void BinaryFile::share(const std::string &username)
{
    if(username.empty())
        throw FileSystemException("Username invalid pentru partajare!");

    auto it=std::find(this->m_sharedWith.begin(),this->m_sharedWith.end(),username);
    if(it==this->m_sharedWith.end())
        this->m_sharedWith.push_back(username);  
}

void BinaryFile::revokeAccess(const std::string &username)
{
    auto it=std::find(this->m_sharedWith.begin(),this->m_sharedWith.end(),username);
    if(it==this->m_sharedWith.end())
        throw AccessDeniedException(username+" nu are acces la acest fisier!");

    this->m_sharedWith.erase(it);   
}

std::vector<std::string> BinaryFile::getSharedWith() const
{
    return this->m_sharedWith;
}

std::string BinaryFile::serialize() const
{
    std::ostringstream oss;
   oss<<File::serialize()<<"|";

   for(int i = 0 ;i<(int)this->m_sharedWith.size();i++)
   {
    oss<<this->m_sharedWith[i];
    if(i<(int)this->m_sharedWith.size()-1)
     oss<<",";
   }
  return oss.str(); 
}

void BinaryFile::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|')) 
        tokens.push_back(token);

    if (tokens.size() < 7) 
        throw FileSystemException("Date invalide pentru deserializare TextFile!");

    std::string baseData;
    for(int i =0;i<7;i++)
    {
         baseData+=tokens[i];
         if(i<6)
          baseData+="|";  
    }
    File::deserialize(baseData);


    if(tokens.size()>7 && !tokens[7].empty())
    {
        std::istringstream sharedStream(tokens[7]);
        std::string user;
        while(std::getline(sharedStream,user,','))
            this->m_sharedWith.push_back(user);
    }
}

bool BinaryFile::operator==(const BinaryFile &other) const
{
    return this->m_name==other.m_name  && this->m_ownerUser==other.m_ownerUser && this->m_extension==other.m_extension;
}

std::ostream &operator<<(std::ostream &os, const BinaryFile &file)
{
   os << file.getIcon() << " " << file.m_name << file.m_extension<< " (" << file.m_size << " bytes)";
   return os;
}
