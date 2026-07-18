#pragma once
#include "FileSystemEntity.h"
#include "../interfaces/ISearchable.h"
#include "BPlusTree.h"
#include "../services/SortManager.h"
#include <vector>
#include <memory>
#include <algorithm>

class Folder : public FileSystemEntity, public iSearchable
{
protected:
    std::vector<std::shared_ptr<FileSystemEntity>> m_children;
    BPlusTree<std::string, std::shared_ptr<FileSystemEntity>> m_index;
    Folder* m_parent;
    std::size_t m_cachedSize = 0;

public:
    Folder(const std::string& name, const std::string& ownerUser, Folder* parent = nullptr);
    Folder(const Folder& other);
    Folder(Folder&& other) noexcept;

    Folder& operator=(const Folder& other);
    Folder& operator=(Folder&& other) noexcept;

    virtual ~Folder() = default;

    Folder* getParent() const { return m_parent; }
    int getChildCount() const { return m_children.size(); }
    std::vector<std::shared_ptr<FileSystemEntity>> getChildren() const { return m_children; }

    void setParent(Folder* parent) { m_parent = parent; }
    void setCachedSize(std::size_t size) { m_cachedSize = size; }

    void addChild(std::shared_ptr<FileSystemEntity> entity);
    void removeChild(const std::string& name);
    std::shared_ptr<FileSystemEntity> findChild(const std::string& name) const;
    bool hasChild(const std::string& name) const;

    std::string getAbsolutePath() const;

    std::size_t getSize() const override;
    bool isFolder() const override { return true; }

    void display() const override;
    std::string getIcon() const override;

    std::vector<std::string> search(const std::string& text) const override;
    bool contains(const std::string& data) const override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    bool operator==(const Folder& other) const;
    bool operator<(const Folder& other) const;

    std::shared_ptr<FileSystemEntity> operator[](const std::string& name) const;

    friend std::ostream& operator<<(std::ostream& os, const Folder& folder);

    void sortChildrenRecursive(SortManager::SortCrit crit);
};
