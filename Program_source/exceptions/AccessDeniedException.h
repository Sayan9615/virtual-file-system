#pragma once
#include "FileSystemException.h"

class AccessDeniedException : public FileSystemException
{
    public: AccessDeniedException(const std::string& path): FileSystemException("Acces refuzat "+path){}
    
};
