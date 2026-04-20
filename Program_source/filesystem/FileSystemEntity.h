#pragma once
#include "../interfaces/ISerializable.h"
#include "../interfaces/IDisplayable.h"
#include "../interfaces/ILogger.h"
#include "../services/PermissionManager.h"
#include <string>
#include <memory>
#include <ctime>
#include <vector>

class FileSystemEntity : public iSerializable, public iDisplayable {
protected:
    int m_id = 0;
    std::string m_name;
    std::string m_ownerUser;
    std::string m_ownerGroup;
    std::time_t m_createdAt;
    std::time_t m_modifiedAt;
    PermissionManager m_permissions;
    iLogger* m_logger = nullptr;

public:
    FileSystemEntity(const std::string& name, const std::string& ownerUser, 
                     const std::string& ownerGroup, iLogger* logger = nullptr);
    FileSystemEntity(const FileSystemEntity& other);
    FileSystemEntity(FileSystemEntity&& other) noexcept;

    FileSystemEntity& operator=(const FileSystemEntity& other);
    FileSystemEntity& operator=(FileSystemEntity&& other) noexcept;

    virtual ~FileSystemEntity() = default;

    // Getteri
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    std::string getName() const { return m_name; }
    std::string getOwnerUser() const { return m_ownerUser; }
    std::string getOwnerGroup() const { return m_ownerGroup; }
    std::time_t getCreatedAt() const { return m_createdAt; }
    std::time_t getModifiedAt() const { return m_modifiedAt; }
    PermissionManager& getPermissions() { return m_permissions; }
    const PermissionManager& getPermissions() const { return m_permissions; }

    void setLogger(iLogger* logger) { m_logger = logger; }

    // Setteri
    void setName(const std::string& name);
    void setOwnerUser(const std::string& user);
    void setOwnerGroup(const std::string& group);

    virtual std::size_t getSize() const = 0;
    virtual bool isFolder() const = 0;

    virtual void display() const override;
    virtual std::string getIcon() const override;
    virtual std::string serialize() const override;
    virtual void deserialize(const std::string& data) override;

    bool operator==(const FileSystemEntity& other) const;
    bool operator!=(const FileSystemEntity& other) const;
    bool operator<(const FileSystemEntity& other) const;

    friend std::ostream& operator<<(std::ostream& os, const FileSystemEntity& entity);
};