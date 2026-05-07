#pragma once
#include "AppException.h"

class DbException : public AppException
{
    public:
        DbException(const std::string& message):AppException("[DB] "+message){}
};