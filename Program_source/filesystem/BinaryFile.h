#pragma once
#include "File.h"
#include <vector>

class BinaryFile : public File
{
private:
    std::vector<unsigned char> m_data;

public:
    BinaryFile(const std::string& name, const std::string& ownerUser, const std::string& extension);
    BinaryFile(const BinaryFile& other);
    BinaryFile(BinaryFile&& other) noexcept;

    BinaryFile& operator=(const BinaryFile& other);
    BinaryFile& operator=(BinaryFile&& other) noexcept;

    ~BinaryFile() = default;

    std::vector<unsigned char> getData() const { return m_data; }

    std::string read() const override;
    void write(const std::string& content) override;

    void display() const override;
    std::string getIcon() const override;

    std::vector<std::string> search(const std::string& text) const override;
    bool contains(const std::string& data) const override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    bool operator==(const BinaryFile& other) const;

    friend std::ostream& operator<<(std::ostream& os, const BinaryFile& file);
};