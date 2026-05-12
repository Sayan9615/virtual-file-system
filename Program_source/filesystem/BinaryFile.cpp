#include "BinaryFile.h"
#include "../exceptions/FileSystemException.h"
#include <iostream>
#include <sstream>

BinaryFile::BinaryFile(const std::string& name, const std::string& ownerUser, const std::string& extension)
    : File(name, ownerUser, extension)
{
}

BinaryFile::BinaryFile(const BinaryFile& other)
    : File(other), m_data(other.m_data)
{
}

BinaryFile::BinaryFile(BinaryFile&& other) noexcept
    : File(std::move(other)), m_data(std::move(other.m_data))
{
}

BinaryFile& BinaryFile::operator=(const BinaryFile& other)
{
    if (this != &other) {
        File::operator=(other);
        m_data = other.m_data;
    }
    return *this;
}

BinaryFile& BinaryFile::operator=(BinaryFile&& other) noexcept
{
    if (this != &other) {
        File::operator=(std::move(other));
        m_data = std::move(other.m_data);
    }
    return *this;
}

std::string BinaryFile::read() const
{
    return "BinaryFile: " + m_name + m_extension + " (" + std::to_string(m_size) + " bytes)  continut binar, nu poate fi afisat";
}

void BinaryFile::write(const std::string& content)
{
    m_data.assign(content.begin(), content.end());
    m_size = m_data.size();
    m_modifiedAt = std::time(nullptr);
}

void BinaryFile::display() const
{
    std::cout << getIcon() << " " << m_name << m_extension
              << " | owner: " << m_ownerUser
              << " | size: " << m_size << " bytes\n";
}

std::string BinaryFile::getIcon() const
{
    if (m_extension == ".docx" || m_extension == ".doc") return "[WORD]";
    if (m_extension == ".pptx" || m_extension == ".ppt") return "[PPT]";
    if (m_extension == ".jpg"  || m_extension == ".png") return "[IMG]";
    if (m_extension == ".pdf") return "[PDF]";
    return "[BIN]";
}

std::vector<std::string> BinaryFile::search(const std::string& text) const
{
    std::vector<std::string> results;
    if (contains(text))
        results.push_back(m_name + m_extension);
    return results;
}

bool BinaryFile::contains(const std::string& data) const
{
    return m_name.find(data) != std::string::npos;
}

std::string BinaryFile::serialize() const
{
    return File::serialize();
}

void BinaryFile::deserialize(const std::string& data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    if (tokens.size() < 6)
        throw FileSystemException("Date invalide pentru deserializare BinaryFile!");

    std::string baseData;
    for (int i = 0; i < 6; i++) {
        baseData += tokens[i];
        if (i < 5) baseData += "|";
    }
    File::deserialize(baseData);
}

bool BinaryFile::operator==(const BinaryFile& other) const
{
    return m_name == other.m_name && m_ownerUser == other.m_ownerUser && m_extension == other.m_extension;
}

std::ostream& operator<<(std::ostream& os, const BinaryFile& file)
{
    os << file.getIcon() << " " << file.m_name << file.m_extension << " (" << file.m_size << " bytes)";
    return os;
}