#pragma once
#include <exception>
#include <string>

class AppException : public std::exception
{
    protected:
        std::string m_message;
    
    public:

        virtual ~AppException()=default;

        AppException(const std::string& message): m_message(message){}

        virtual const char* what() const noexcept override
        {
            return m_message.c_str();
        }    

};