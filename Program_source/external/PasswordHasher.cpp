#include "PasswordHasher.h"
#include <sodium.h>
#include <stdexcept>

bool PasswordHasher::initialize_sodium() {
    return sodium_init() >= 0;
}

std::string PasswordHasher::hashPassword(const std::string& password) {
    char hashed_password[crypto_pwhash_STRBYTES];

if (crypto_pwhash_str
    (hashed_password, password.c_str(), strlen(password.c_str()),
     crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
    // Introducere exceptie aici
}
    return std::string(hashed_password);
}

bool PasswordHasher::verifyPassword(const std::string& password, const std::string& storedHash) {
    if (crypto_pwhash_str_verify
    (storedHash.c_str(), password.c_str(), strlen(password.c_str())) != 0) {
    return false;
    }
    return true;
}