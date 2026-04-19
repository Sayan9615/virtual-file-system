#pragma once
#include "../filesystem/FileSystemEntity.h"
#include "../filesystem/File.h"
#include <vector>
#include <memory>
#include <algorithm>


struct CompareByNameAsc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getName()<b->getName();
    }

};

struct CompareByNameDesc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getName()>b->getName();
    }

};

struct CompareBySizeAsc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getSize()<b->getSize();
    }

};

struct CompareBySizeDesc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getSize()>b->getSize();
    }

};

struct CompareByDateAsc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getCreatedAt()<b->getCreatedAt();
    }

};

struct CompareByDateDesc
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->getCreatedAt()>b->getCreatedAt();
    }

};

struct CompareByType
{
    bool operator()(const std::shared_ptr<FileSystemEntity>&a ,const std::shared_ptr<FileSystemEntity>&b) const
    {
        return a->isFolder()>b->isFolder();
    }

};

class SortManager
{
    public:
        enum class SortCrit{NAME_ASC,NAME_DESC,SIZE_ASC,SIZE_DESC,DATE_ASC,DATE_DESC,TYPE};

    static std::vector<std::shared_ptr<FileSystemEntity>> sort(std::vector<std::shared_ptr<FileSystemEntity>> entities,SortCrit criterium);
    static std::vector<std::shared_ptr<FileSystemEntity>> sortByName(std::vector<std::shared_ptr<FileSystemEntity>> entities,bool ascending=true);
    static std::vector<std::shared_ptr<FileSystemEntity>> sortBySize(std::vector<std::shared_ptr<FileSystemEntity>> entities,bool ascending=true);
    static std::vector<std::shared_ptr<FileSystemEntity>> sortByDate(std::vector<std::shared_ptr<FileSystemEntity>> entities,bool ascending=true);


};