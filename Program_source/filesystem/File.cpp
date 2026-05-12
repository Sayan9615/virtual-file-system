#include "File.h"
#include "../exceptions/FileSystemException.h"
#include <iostream>
#include <sstream>

File::File(const std::string& name, const std::string& ownerUser, const std::string& extension)
    : FileSystemEntity(name, ownerUser), m_extension(extension), m_size(0)
{
    if (extension.empty())
        throw FileSystemException("Extensia trebuie specificata!");
}

File::File(const File& other)
    : FileSystemEntity(other), m_extension(other.m_extension), m_size(other.m_size)
{
}

File::File(File&& other) noexcept
    : FileSystemEntity(std::move(other)), m_extension(std::move(other.m_extension)), m_size(other.m_size)
{
    other.m_size = 0;
}

File& File::operator=(const File& other)
{
    if (this != &other) {
        FileSystemEntity::operator=(other);
        m_extension = other.m_extension;
        m_size = other.m_size;
    }
    return *this;
}

File& File::operator=(File&& other) noexcept
{
    if (this != &other) {
        FileSystemEntity::operator=(std::move(other));
        m_extension = std::move(other.m_extension);
        m_size = other.m_size;
        other.m_size = 0;
    }
    return *this;
}

void File::setExtension(const std::string& extension)
{
    if (extension.empty())
        throw FileSystemException("Extensia nu poate fi nula!");
    m_extension = extension;
    m_modifiedAt = std::time(nullptr);
}

std::string File::serialize() const
{
    std::ostringstream oss;
    oss << FileSystemEntity::serialize() << "|" << m_extension << "|" << m_size;
    return oss.str();
}

void File::deserialize(const std::string& data)
{
    FileSystemEntity::deserialize(data);

    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    // FileSystemEntity now produces 4 tokens (name, owner, createdAt, modifiedAt)
    // File adds extension and size -> tokens[4] and tokens[5]
    if (tokens.size() < 6)
        throw FileSystemException("Date invalide pentru deserializare fisier!");

    m_extension = tokens[4];
    m_size = std::stoull(tokens[5]);
}

void File::display() const
{
    std::cout << getIcon() << " " << m_name << m_extension
              << " | owner: " << m_ownerUser
              << " | size: " << m_size << " bytes\n";
}

std::string File::getIcon() const
{
    return "[FILE]";
}

std::vector<std::string> File::search(const std::string& text) const
{
    std::vector<std::string> results;
    if (contains(text))
        results.push_back(m_name + m_extension);
    return results;
}

bool File::contains(const std::string& data) const
{
    return m_name.find(data) != std::string::npos;
}

bool File::operator==(const File& other) const
{
    return m_name == other.m_name && m_extension == other.m_extension && m_ownerUser == other.m_ownerUser;
}

bool File::operator<(const File& other) const
{
    return m_size < other.m_size;
}

bool File::operator>(const File& other) const
{
    return m_size > other.m_size;
}

std::ostream& operator<<(std::ostream& os, const File& file)
{
    os << "[FILE] " << file.m_name << file.m_extension << " (" << file.m_size << " bytes)";
    return os;
}