#pragma once
#include "AppException.h"

class FileSystemException : public AppException
{
    public:
        FileSystemException(const std::string& message): AppException("[FileSystem] "+message) {}
};