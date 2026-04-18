#pragma once
#include "FileSystemException.h"

class FileNotFoundException : public FileSystemException
{
    public:
        FileNotFoundException(const std::string& path):FileSystemException("Fisierul nu a fost gasit: "+path){}
};
