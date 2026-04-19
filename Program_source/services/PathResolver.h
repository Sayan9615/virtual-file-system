#pragma once
#include "../exceptions/FileNotFoundException.h"
#include "../exceptions/FileSystemException.h"
#include "../filesystem/Folder.h"
#include <string>
#include <vector>
#include <memory>

class PathResolver
{
    private:
        std::shared_ptr<Folder> m_root;
        std::string m_lastValidPath;

        std::vector<std::string> splitPath(const std::string&  path) const;

    public:
        PathResolver(std::shared_ptr<Folder>root);
        PathResolver(const PathResolver& other);
        PathResolver(PathResolver&& other)noexcept;

        PathResolver& operator=(const PathResolver& other);
        PathResolver& operator=(PathResolver&& other)noexcept;

        ~PathResolver()=default;

        std::shared_ptr<FileSystemEntity> resolvePath(const std::string& path)const;
    
        bool validatePath(const std::string& path)const;

        std::shared_ptr<Folder> navigateTo(const std::string& path)const;

        std::string getLastValidPath() const {return this->m_lastValidPath;}

        std::string getParentPath(const std::string& path)const;

        std::string getEntityName(const std::string& path);

        std::string buildPath(const std::shared_ptr<FileSystemEntity>& entity) const;

};