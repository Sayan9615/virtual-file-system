#pragma once
#include "AppException.h"

class AuthException : public AppException
{
    public:
        AuthException(const std::string& message):AppException("[Auth] "+message){}
};