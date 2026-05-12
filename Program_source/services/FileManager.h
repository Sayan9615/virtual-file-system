#pragma once
#include "Database.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../filesystem/Folder.h"
#include "../interfaces/ILogger.h"
#include "EventLog.h"
#include "../interfaces/IFileManager.h"
#include <memory>
#include <vector>
#include <string>

class FileManager : public IFileManager {
public:
    FileManager(Database& db, iLogger& logger);

    virtual bool initializeDatabase() override;

    // Creare
    virtual bool createTextFile(const std::string& name, const std::string& ownerUser,
                                const std::string& content, int parentId = 1) override;

    bool createBinaryFile(const std::string& name, const std::string& ownerUser,
                          const std::string& extension, int parentId = 1);

    virtual bool createFolder(const std::string& name, const std::string& ownerUser,
                              int parentId = 1) override;

    // Citire
    std::shared_ptr<TextFile> getTextFile(int id);
    std::vector<std::shared_ptr<FileSystemEntity>> getChildren(int parentId);

    // Update
    bool updateTextFile(int id, const std::string& content, const std::string& username);
    virtual bool renameEntity(int id, const std::string& newName, const std::string& username) override;

    // Stergere
    virtual bool deleteEntity(int id, const std::string& username) override;

    // Permisiuni
    virtual bool checkPermission(int entityId, const std::string& username, const std::string& operation) override;
    virtual bool updateOthersPermissions(int entityId, bool canRead, bool canWrite) override;
    virtual PermissionManager loadPermissions(int entityId) override;

    // Share
    virtual bool shareEntity(int entityId, const std::string& username, bool canRead, bool canWrite) override;
    virtual bool revokeShare(int entityId, const std::string& username) override;

    int getParentId(int entityId) const;

    // Utilitare
    virtual int getRootId() override;
    int getEntityId(const std::string& name, int parentId);
    virtual std::shared_ptr<Folder> buildTree(int folderId, Folder* parent = nullptr,
                                              const std::string& username = "") override;

private:
    Database& db;
    iLogger& logger;

    bool savePermissions(int entityId, const PermissionManager& pm);
};