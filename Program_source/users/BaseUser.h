#pragma once
#include "ISerializable.h"
#include <string>

class BaseUser : public iSerializable
{
protected:
    int id;
    std::string username;

public:
    BaseUser() : id(-1), username("") {}

    BaseUser(int id, const std::string& username)
        : id(id), username(username) {}

    int getId() const {
        return id;
    }

    std::string getUsername() const {
        return username;
    }

    bool isValid() const {
        return id != -1;
    }

    void invalidate() {
        id = -1;
        username = "";
    }

    std::string serialize() const override {return NULL;};

    void deserialize(const std:: string &data) override {};
};