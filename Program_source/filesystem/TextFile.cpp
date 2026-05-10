#include "TextFile.h"
#include "../exceptions/AccessDeniedException.h"
#include "../exceptions/FileSystemException.h"
#include <iostream>
#include <sstream>
#include <algorithm>

static std::string extractExt(const std::string& name) {
    auto p = name.rfind('.');
    return (p != std::string::npos) ? name.substr(p) : ".txt";
}

TextFile::TextFile(const std::string &name, const std::string &ownerUser, const std::string &ownerGroup, const std::string &content)
:File(name,ownerUser,ownerGroup,extractExt(name)),m_content(content)
{
    this->m_size=this->m_content.size();
}

TextFile::TextFile(const TextFile &other)
:File(other),m_content(other.m_content),m_sharedWith(other.m_sharedWith)
{
}

TextFile::TextFile(TextFile &&other) noexcept
:File(std::move(other)),m_content(std::move(other.m_content)),m_sharedWith(std::move(other.m_sharedWith))
{
}

TextFile &TextFile::operator=(const TextFile &other)
{
    if(this!=&other)
    {
        File::operator=(other);
        this->m_content=other.m_content;
        this->m_sharedWith=other.m_sharedWith;
    }
    return *this;
}

TextFile &TextFile::operator=(TextFile &&other) noexcept
{
    if (this != &other) 
    {
        File::operator=(std::move(other));
        m_content    = std::move(other.m_content);
        m_sharedWith = std::move(other.m_sharedWith);
    }
    return *this;
}

std::string TextFile::read() const
{
    return this->m_content;
}

void TextFile::write(const std::string &content)
{
    this->m_content=content;
    this->m_size=this->m_content.size();
    this->m_modifiedAt=std::time(nullptr);
}

void TextFile::display() const
{
    std::cout << getIcon() << " " << m_name << m_extension << " | owner: "   << m_ownerUser<< " | size: "    << m_size << " bytes"<< " | shared: "  << m_sharedWith.size() << " useri"<< std::endl;
}

std::string TextFile::getIcon() const
{
    return this->m_sharedWith.empty()?"[TXT]":"[TXT-SHARED]";
}

std::vector<std::string> TextFile::search(const std::string &text) const
{
   std::vector<std::string> results;
    if (contains(text)) 
        results.push_back(m_name + m_extension);
    
    return results;
}

static bool ciFind(const std::string& hay, const std::string& needle) {
    return std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
        [](char a, char b){ return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }
    ) != hay.end();
}

bool TextFile::contains(const std::string &data) const
{
    return ciFind(this->m_name, data) || ciFind(this->m_content, data);
}

void TextFile::share(const std::string &username)
{
    if(username.empty())
        throw FileSystemException("Username invalid pentru partajare!");

    auto it=std::find(this->m_sharedWith.begin(),this->m_sharedWith.end(),username);
    if(it==this->m_sharedWith.end())
        this->m_sharedWith.push_back(username);    
}

void TextFile::revokeAccess(const std::string &username)
{
    auto it=std::find(this->m_sharedWith.begin(),this->m_sharedWith.end(),username);
    if(it==this->m_sharedWith.end())
        throw AccessDeniedException(username+" nu are acces la acest fisier!");

    this->m_sharedWith.erase(it);    
}

std::vector<std::string> TextFile::getSharedWith() const
{
    return this->m_sharedWith;
}

std::string TextFile::serialize() const
{
   std::ostringstream oss;
   oss<<File::serialize()<<"|"<<this->m_content<<"|";

   for(int i = 0 ;i<(int)this->m_sharedWith.size();i++)
   {
    oss<<this->m_sharedWith[i];
    if(i<(int)this->m_sharedWith.size()-1)
     oss<<",";
   }
  return oss.str(); 
}

void TextFile::deserialize(const std::string &data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|')) 
        tokens.push_back(token);

    if (tokens.size() < 8) 
        throw FileSystemException("Date invalide pentru deserializare TextFile!");

    std::string baseData;
    for(int i =0;i<7;i++)
    {
         baseData+=tokens[i];
         if(i<6)
          baseData+="|";  
    }
    File::deserialize(baseData);

    this->m_content=tokens[7];

    if(tokens.size()>8 && !tokens[8].empty())
    {
        std::istringstream sharedStream(tokens[8]);
        std::string user;
        while(std::getline(sharedStream,user,','))
            this->m_sharedWith.push_back(user);
    }
    
}

bool TextFile::operator==(const TextFile &other) const
{
    return this->m_name==other.m_name && this->m_content==other.m_content && this->m_ownerUser==other.m_ownerUser;
}

bool TextFile::operator+=(const std::string &content)
{
    this->m_content+=content;
    this->m_size=this->m_content.size();
    this->m_modifiedAt=std::time(nullptr);
    return true;
}

std::ostream &operator<<(std::ostream &os, const TextFile &file)
{
    os<<"[TXT]"<<file.m_name<<file.m_extension<<" ("<<file.m_size<<" bytes)"<<" content"<<file.m_content.substr(0,20)<<(file.m_content.size()>20?"...":"");
    return os;
}
