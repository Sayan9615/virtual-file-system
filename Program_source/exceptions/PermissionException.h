#pragma once
#include "AppException.h"

class PermissionException : public AppException
{
    public:
        PermissionException(const std::string& message):AppException("[Permission ]"+message){}
};