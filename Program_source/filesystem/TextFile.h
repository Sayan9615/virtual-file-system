#pragma once
#include "File.h"
#include "../interfaces/IShareable.h"
#include <string>
#include <vector>

class TextFile : public File ,public iShareable
{
    private:
        std::string m_content;
        std::vector<std::string>m_sharedWith;

    public:
    TextFile(const std::string& name,const std::string& ownerUser,const std::string& ownerGroup,const std::string& content = "");
    TextFile(const TextFile& other);
    TextFile(TextFile&& other) noexcept;

    // operatori de atribuire
    TextFile& operator=(const TextFile& other);
    TextFile& operator=(TextFile&& other) noexcept;

    ~TextFile() = default;


    std::string getContent() const { return m_content; }

    
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

    
    bool operator==(const TextFile& other) const;
    bool operator+=(const std::string& content); // append continut

    friend std::ostream& operator<<(std::ostream& os, const TextFile& file);    
};
