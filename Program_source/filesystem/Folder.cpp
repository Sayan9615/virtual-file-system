#include "Folder.h"
#include "../exceptions/FileSystemException.h"
#include "../exceptions/FileNotFoundException.h"
#include <iostream>
#include <sstream>
#include <numeric>

Folder::Folder(const std::string& name, const std::string& ownerUser, Folder* parent)
    : FileSystemEntity(name, ownerUser), m_index(3), m_parent(parent)
{
}

Folder::Folder(const Folder& other)
    : FileSystemEntity(other), m_children(other.m_children), m_index(3), m_parent(other.m_parent)
{
    for (auto& child : m_children)
        m_index.insert(child->getName(), child);
}

Folder::Folder(Folder&& other) noexcept
    : FileSystemEntity(std::move(other)), m_children(std::move(other.m_children)),
      m_index(std::move(other.m_index)), m_parent(other.m_parent)
{
    other.m_parent = nullptr;
}

Folder& Folder::operator=(const Folder& other)
{
    if (this != &other) {
        FileSystemEntity::operator=(other);
        m_children = other.m_children;
        m_parent = other.m_parent;
        m_index = BPlusTree<std::string, std::shared_ptr<FileSystemEntity>>(3);
        for (auto& child : m_children)
            m_index.insert(child->getName(), child);
    }
    return *this;
}

Folder& Folder::operator=(Folder&& other) noexcept
{
    if (this != &other) {
        FileSystemEntity::operator=(std::move(other));
        m_children = std::move(other.m_children);
        m_index = std::move(other.m_index);
        m_parent = other.m_parent;
        other.m_parent = nullptr;
    }
    return *this;
}

void Folder::addChild(std::shared_ptr<FileSystemEntity> entity)
{
    if (!entity)
        throw FileSystemException("Nu pot adauga ceva ce e null in folder!");

    if (hasChild(entity->getName()))
        throw FileSystemException("Exista deja un fisier/folder cu numele: " + entity->getName());

    m_children.push_back(entity);
    m_index.insert(entity->getName(), entity);
    m_modifiedAt = std::time(nullptr);
}

void Folder::removeChild(const std::string& name)
{
    for (int i = 0; i < (int)m_children.size(); i++) {
        if (m_children[i]->getName() == name) {
            m_children.erase(m_children.begin() + i);
            m_index.remove(name);
            m_modifiedAt = std::time(nullptr);
            return;
        }
    }
    throw FileNotFoundException(getAbsolutePath() + "/" + name);
}

std::shared_ptr<FileSystemEntity> Folder::findChild(const std::string& name) const
{
    auto result = m_index.search(name);
    if (result.has_value())
        return result.value();
    throw FileNotFoundException(getAbsolutePath() + "/" + name);
}

bool Folder::hasChild(const std::string& name) const
{
    return m_index.search(name).has_value();
}

std::string Folder::getAbsolutePath() const
{
    if (m_parent == nullptr)
        return "/" + m_name;
    return m_parent->getAbsolutePath() + "/" + m_name;
}

std::size_t Folder::getSize() const
{
    if (m_children.empty())
        return m_cachedSize;

    std::size_t total = 0;
    for (const auto& child : m_children)
        total += child->getSize();
    return total;
}

void Folder::display() const
{
    std::cout << getIcon() << " " << m_name << " | owner: " << m_ownerUser
              << " | elemente: " << m_children.size() << " | size: " << getSize() << " bytes\n";
    for (const auto& child : m_children) {
        std::cout << " ";
        child->display();
    }
}

std::string Folder::getIcon() const
{
    return m_children.empty() ? "[DIR-EMPTY]" : "[DIR]";
}

std::vector<std::string> Folder::search(const std::string& text) const
{
    std::vector<std::string> results;
    for (const auto& child : m_children) {
        if (child->getName().find(text) != std::string::npos)
            results.push_back(getAbsolutePath() + "/" + child->getName());

        auto* searchable = dynamic_cast<iSearchable*>(child.get());
        if (searchable) {
            auto result2 = searchable->search(text);
            results.insert(results.end(), result2.begin(), result2.end());
        }
    }
    return results;
}

bool Folder::contains(const std::string& data) const
{
    return m_name.find(data) != std::string::npos || hasChild(data);
}

std::string Folder::serialize() const
{
    std::ostringstream oss;
    oss << FileSystemEntity::serialize() << "|" << m_children.size();
    return oss.str();
}

void Folder::deserialize(const std::string& data)
{
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|'))
        tokens.push_back(token);

    if (tokens.size() < 5)
        throw FileSystemException("Date invalide pentru folder!");

    std::string baseData;
    for (int i = 0; i < 4; i++) {
        baseData += tokens[i];
        if (i < 3) baseData += "|";
    }
    FileSystemEntity::deserialize(baseData);
}

bool Folder::operator==(const Folder& other) const
{
    return m_name == other.m_name && m_ownerUser == other.m_ownerUser;
}

bool Folder::operator<(const Folder& other) const
{
    return m_name < other.m_name;
}

std::shared_ptr<FileSystemEntity> Folder::operator[](const std::string& name) const
{
    return findChild(name);
}

std::ostream& operator<<(std::ostream& os, const Folder& folder)
{
    os << "[DIR] " << folder.m_name << " (" << folder.m_children.size() << " elemente)";
    return os;
}

void Folder::sortChildrenRecursive(SortManager::SortCrit crit)
{
    m_children = SortManager::sort(m_children, crit);
    for (auto& child : m_children) {
        if (child->isFolder()) {
            auto folder = std::dynamic_pointer_cast<Folder>(child);
            if (folder) folder->sortChildrenRecursive(crit);
        }
    }
}
