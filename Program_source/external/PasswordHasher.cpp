#include "PasswordHasher.h"
#include <sodium.h>
#include <stdexcept>

bool PasswordHasher::initialize() {
    return sodium_init() >= 0;
}

std::string PasswordHasher::hashPassword(const std::string& password) {
    char hashed[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hashed,
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        throw std::runtime_error("Nu s-a putut genera hash-ul parolei.");
    }

    return std::string(hashed);
}

bool PasswordHasher::verifyPassword(const std::string& password,
                                    const std::string& storedHash) {
    return crypto_pwhash_str_verify(
               storedHash.c_str(),
               password.c_str(),
               password.size()) == 0;
}