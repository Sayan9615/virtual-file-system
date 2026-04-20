#include "FileSystemEntity.h"
#include "../services/EventLog.h"
#include <iostream>
#include <sstream>
#include "../exceptions/FileSystemException.h"

FileSystemEntity::FileSystemEntity(const std::string& name,
                                   const std::string& ownerUser,
                                   const std::string& ownerGroup,
                                   iLogger* logger)
    : m_name(name), m_ownerUser(ownerUser), m_ownerGroup(ownerGroup),
      m_createdAt(std::time(nullptr)), m_modifiedAt(std::time(nullptr)),
      m_logger(logger) {

    m_permissions.addPermission(
        std::make_shared<OwnerPermission>(ownerUser, true, true)
    );
    {
        Group g(ownerGroup);
        g.addMember(ownerUser);
        auto gp = std::make_shared<GroupPermission>(true, false);
        gp->addGroup(g);
        m_permissions.addPermission(gp);
    }

    if (m_logger)
        EventLog(EventType::FILE_UPLOAD, ownerUser, "Entitate creata: " + name, *m_logger);
}

// constructori de copiere/move — adauga m_logger
FileSystemEntity::FileSystemEntity(const FileSystemEntity& other)
    : m_name(other.m_name), m_ownerUser(other.m_ownerUser),
      m_ownerGroup(other.m_ownerGroup), m_createdAt(other.m_createdAt),
      m_modifiedAt(other.m_modifiedAt), m_permissions(other.m_permissions),
      m_logger(other.m_logger) {}

FileSystemEntity::FileSystemEntity(FileSystemEntity&& other) noexcept
    : m_name(std::move(other.m_name)), m_ownerUser(std::move(other.m_ownerUser)),
      m_ownerGroup(std::move(other.m_ownerGroup)), m_createdAt(other.m_createdAt),
      m_modifiedAt(other.m_modifiedAt), m_permissions(std::move(other.m_permissions)),
      m_logger(other.m_logger) {}

FileSystemEntity& FileSystemEntity::operator=(const FileSystemEntity& other) {
    if (this != &other) {
        m_name = other.m_name;
        m_ownerUser = other.m_ownerUser;
        m_ownerGroup = other.m_ownerGroup;
        m_createdAt = other.m_createdAt;
        m_modifiedAt = other.m_modifiedAt;
        m_permissions = other.m_permissions;
        m_logger = other.m_logger;
    }
    return *this;
}

FileSystemEntity& FileSystemEntity::operator=(FileSystemEntity&& other) noexcept {
    if (this != &other) {
        m_name = std::move(other.m_name);
        m_ownerUser = std::move(other.m_ownerUser);
        m_ownerGroup = std::move(other.m_ownerGroup);
        m_createdAt = other.m_createdAt;
        m_modifiedAt = other.m_modifiedAt;
        m_permissions = std::move(other.m_permissions);
        m_logger = other.m_logger;
    }
    return *this;
}

void FileSystemEntity::setName(const std::string& name) {
    if (name.empty())
        throw FileSystemException("Numele nu poate fi gol!");

    if (m_logger)
        EventLog(EventType::FILE_UPLOAD, m_ownerUser, 
                 "Redenumit: " + m_name + " -> " + name, *m_logger);

    m_name = name;
    m_modifiedAt = std::time(nullptr);
}

void FileSystemEntity::setOwnerUser(const std::string& user) {
    if (m_logger)
        EventLog(EventType::FILE_SHARE, m_ownerUser,
                 "Owner schimbat: " + m_name + " -> " + user, *m_logger);
    m_ownerUser = user;
}

void FileSystemEntity::setOwnerGroup(const std::string& group) {
    if (m_logger)
        EventLog(EventType::FILE_SHARE, m_ownerUser,
                 "Grup schimbat: " + m_name + " -> " + group, *m_logger);
    m_ownerGroup = group;
}

// restul metodelor raman identice
void FileSystemEntity::display() const {
    std::cout << getIcon() << " " << m_name << " | owner: " << m_ownerUser
              << " | group: " << m_ownerGroup << " | size: " << getSize() << " bytes\n";
}

std::string FileSystemEntity::getIcon() const {
    return isFolder() ? "[DIR]" : "[FILE]";
}

std::string FileSystemEntity::serialize() const {
    std::ostringstream oss;
    oss << m_name << "|" << m_ownerUser << "|" << m_ownerGroup
        << "|" << m_createdAt << "|" << m_modifiedAt;
    return oss.str();
}

void FileSystemEntity::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    if (tokens.size() < 5)
        throw FileSystemException("Date invalide pentru deserializare entitate!");

    m_name = tokens[0];
    m_ownerUser = tokens[1];
    m_ownerGroup = tokens[2];
    m_createdAt = std::stoll(tokens[3]);
    m_modifiedAt = std::stoll(tokens[4]);
}

bool FileSystemEntity::operator==(const FileSystemEntity& other) const {
    return m_name == other.m_name && m_ownerGroup == other.m_ownerGroup;
}

bool FileSystemEntity::operator!=(const FileSystemEntity& other) const {
    return !(*this == other);
}

bool FileSystemEntity::operator<(const FileSystemEntity& other) const {
    return m_name < other.m_name;
}

std::ostream& operator<<(std::ostream& os, const FileSystemEntity& entity) {
    os << entity.getIcon() << " " << entity.m_name << " (" << entity.m_ownerUser << ")";
    return os;
}