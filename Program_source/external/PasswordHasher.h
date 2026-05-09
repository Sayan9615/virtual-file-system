#pragma once
#include <string>

class PasswordHasher {
public:
    static bool initialize_sodium();

    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string& password, const std::string& storedHash);
};