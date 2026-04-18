#pragma once
#include "File.h"
#include "../interfaces/IShareable.h"
#include <vector>

class BinaryFile: public File, public iShareable
{
     private:
        std::vector<unsigned char>m_data;
        std::vector<std::string>m_sharedWith;

    public:
    BinaryFile(const std::string& name,const std::string& ownerUser,const std::string& ownerGroup,const std::string& extension);
    BinaryFile(const BinaryFile& other);
    BinaryFile(BinaryFile&& other) noexcept;

    // operatori de atribuire
   BinaryFile& operator=(const BinaryFile& other);
   BinaryFile& operator=(BinaryFile&& other) noexcept;

    ~BinaryFile() = default;


    std::vector<unsigned char> getData() const { return this->m_data; }

    
    std::string read() const override;
    void write(const std::string& content) override;

    
    void display()  const override;
    std::string getIcon()  const override;

   
    std::vector<std::string> search(const std::string& text) const override;
    bool  contains(const std::string& data) const override;

    
    void share(const std::string& username) override;
    void revokeAccess(const std::string& username) override;
    std::vector<std::string> getSharedWith() const override;

    
    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    
    bool operator==(const BinaryFile& other) const;

    friend std::ostream& operator<<(std::ostream& os, const BinaryFile& file);
};
