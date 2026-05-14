#pragma once
#include "FileSystemEntity.h"
#include "../interfaces/ISearchable.h"
#include <string>

class File : public FileSystemEntity, public iSearchable
{
protected:
    std::string m_extension;
    std::size_t m_size;

public:
    File(const std::string& name, const std::string& ownerUser, const std::string& extension = ".txt");
    File(const File& other);
    File(File&& other) noexcept;

    File& operator=(const File& other);
    File& operator=(File&& other) noexcept;

    virtual ~File() = default;

    std::string getExtension() const { return m_extension; }
    std::size_t getSize() const { return m_size; }

    void setExtension(const std::string& extension);
    void setSize(std::size_t size) { m_size = size; }

    bool isFolder() const override { return false; }

    virtual std::string read() const = 0;
    virtual void write(const std::string& content) = 0;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    void display() const override;
    std::string getIcon() const override;

    std::vector<std::string> search(const std::string& text) const override;
    bool contains(const std::string& data) const override;

    bool operator==(const File& other) const;
    bool operator<(const File& other) const;
    bool operator>(const File& other) const;

    friend std::ostream& operator<<(std::ostream& os, const File& file);
};
