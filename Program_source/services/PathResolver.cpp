#include "PathResolver.h"
#include <sstream>

std::vector<std::string> PathResolver::splitPath(const std::string &path) const
{
    std::vector<std::string> tokens;
    std::istringstream iss(path);
    std::string token;

    while(std::getline(iss,token,'/'))
        if(!token.empty())
            tokens.push_back(token);

    return tokens;        
}

PathResolver::PathResolver(std::shared_ptr<Folder> root)
:m_root(root),m_lastValidPath("/")
{
}

PathResolver::PathResolver(const PathResolver &other)
:m_root(other.m_root),m_lastValidPath(other.m_lastValidPath)
{
}

PathResolver::PathResolver(PathResolver &&other) noexcept
:m_root(std::move(other.m_root)),m_lastValidPath(std::move(other.m_lastValidPath))
{
}

PathResolver &PathResolver::operator=(const PathResolver &other)
{
   if(this!=&other)
   {
    this->m_root=other.m_root;
    this->m_lastValidPath=other.m_lastValidPath;
   }
   return *this;
}

PathResolver &PathResolver::operator=(PathResolver &&other) noexcept
{
    if(this!=&other)
   {
    this->m_root=std::move(other.m_root);
    this->m_lastValidPath=std::move(other.m_lastValidPath);
   }
   return *this;
}

std::shared_ptr<FileSystemEntity> PathResolver::resolvePath(const std::string &path) const
{
    if(path.empty() || path=="/")
        return this->m_root;

    auto tokens=splitPath(path);
    std::shared_ptr<FileSystemEntity> current=this->m_root;
    
    for(const auto& token:tokens)
    {
        auto* folder =dynamic_cast<Folder*>(current.get());
        if(!folder)
            throw FileNotFoundException(path);
         
        current=folder->findChild(token);    
    }

    return current;
}

bool PathResolver::validatePath(const std::string &path) const
{
    //laborator exceptii+curs
    try{

        resolvePath(path);
    }
    catch(...)
       { return false;}
}

std::shared_ptr<Folder> PathResolver::navigateTo(const std::string &path) const
{
    auto entity=resolvePath(path);

    auto folder=std::dynamic_pointer_cast<Folder>(entity);

    if(!folder)
        throw FileSystemException(path+" nu e folder!");
    
    const_cast<PathResolver*>(this)->m_lastValidPath=path;
    
    return folder;
}

std::string PathResolver::getParentPath(const std::string &path) const
{
    if(path.empty()|| path=="/")
        return "/";
    
    auto lastSlash=path.rfind('/');
    if(lastSlash==std::string::npos || lastSlash==0)
        return "/";

    return path.substr(0,lastSlash);        
}

std::string PathResolver::getEntityName(const std::string &path)
{
 if(path.empty()|| path=="/")
        return "/";
    
    auto lastSlash=path.rfind('/');
    if(lastSlash==std::string::npos)
        return path;
        
    return path.substr(lastSlash+1); 
}

std::string PathResolver::buildPath(const std::shared_ptr<FileSystemEntity> &entity) const
{
    if(!entity)
        return "/";
    
    auto* folder=dynamic_cast<Folder*>(entity.get());
    if(folder)
        return folder->getAbsolutePath();
        
    return "/"+ entity->getName();       
}
