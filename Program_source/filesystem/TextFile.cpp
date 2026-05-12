#include "TextFile.h"
#include "../exceptions/FileSystemException.h"
#include <iostream>
#include <sstream>
#include <algorithm>

static std::string extractExt(const std::string& name) {
    auto p = name.rfind('.');
    return (p != std::string::npos) ? name.substr(p) : ".txt";
}

TextFile::TextFile(const std::string& name, const std::string& ownerUser, const std::string& content)
    : File(name, ownerUser, extractExt(name)), m_content(content)
{
    m_size = m_content.size();
}

TextFile::TextFile(const TextFile& other)
    : File(other), m_content(other.m_content)
{
}

TextFile::TextFile(TextFile&& other) noexcept
    : File(std::move(other)), m_content(std::move(other.m_content))
{
}

TextFile& TextFile::operator=(const TextFile& other)
{
    if (this != &other) {
        File::operator=(other);
        m_content = other.m_content;
    }
    return *this;
}

TextFile& TextFile::operator=(TextFile&& other) noexcept
{
    if (this != &other) {
        File::operator=(std::move(other));
        m_content = std::move(other.m_content);
    }
    return *this;
}

std::string TextFile::read() const
{
    return m_content;
}

void TextFile::write(const std::string& content)
{
    m_content = content;
    m_size = m_content.size();
    m_modifiedAt = std::time(nullptr);
}

void TextFile::display() const
{
    std::cout << getIcon() << " " << m_name << m_extension
              << " | owner: " << m_ownerUser
              << " | size: " << m_size << " bytes\n";
}

std::string TextFile::getIcon() const
{
    return "[TXT]";
}

std::vector<std::string> TextFile::search(const std::string& text) const
{
    std::vector<std::string> results;
    if (contains(text))
        results.push_back(m_name + m_extension);
    return results;
}

static bool ciFind(const std::string& hay, const std::string& needle) {
    return std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
        }
    ) != hay.end();
}

bool TextFile::contains(const std::string& data) const
{
    return ciFind(m_name, data) || ciFind(m_content, data);
}

std::string TextFile::serialize() const
{
    std::ostringstream oss;
    oss << File::serialize() << "|" << m_content;
    return oss.str();
}

void TextFile::deserialize(const std::string& data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    if (tokens.size() < 7)
        throw FileSystemException("Date invalide pentru deserializare TextFile!");

    std::string baseData;
    for (int i = 0; i < 6; i++) {
        baseData += tokens[i];
        if (i < 5) baseData += "|";
    }
    File::deserialize(baseData);

    m_content = tokens[6];
}

bool TextFile::operator==(const TextFile& other) const
{
    return m_name == other.m_name && m_content == other.m_content && m_ownerUser == other.m_ownerUser;
}

bool TextFile::operator+=(const std::string& content)
{
    m_content += content;
    m_size = m_content.size();
    m_modifiedAt = std::time(nullptr);
    return true;
}

std::ostream& operator<<(std::ostream& os, const TextFile& file)
{
    os << "[TXT] " << file.m_name << file.m_extension << " (" << file.m_size << " bytes) content: "
       << file.m_content.substr(0, 20) << (file.m_content.size() > 20 ? "..." : "");
    return os;
}