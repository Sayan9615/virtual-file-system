#pragma once
#include "Database.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../filesystem/Folder.h"
#include "../filesystem/SharedFolder.h"
#include "../interfaces/ILogger.h"
#include "EventLog.h"
#include <memory>
#include <vector>
#include <string>

class FileManager {
public:
    FileManager(Database& db, iLogger& logger);

    bool initializeDatabase();

    // Creare
    bool createTextFile(const std::string& name, const std::string& ownerUser,
                        const std::string& ownerGroup, const std::string& content,
                        int parentId = 1);

    bool createBinaryFile(const std::string& name, const std::string& ownerUser,
                          const std::string& ownerGroup, const std::string& extension,
                          int parentId = 1);

    bool createFolder(const std::string& name, const std::string& ownerUser,
                      const std::string& ownerGroup, int parentId = 1);

    bool createSharedFolder(const std::string& name, const std::string& ownerUser,
                            const std::string& ownerGroup, bool isPublic = false,
                            int parentId = 1);

    // Citire
    std::shared_ptr<TextFile> getTextFile(int id);
    std::shared_ptr<Folder> getFolder(int id);
    std::vector<std::shared_ptr<FileSystemEntity>> getChildren(int parentId);

    // Update
    bool updateTextFile(int id, const std::string& content, const std::string& username);
    bool renameEntity(int id, const std::string& newName, const std::string& username);

    // Stergere
    bool deleteEntity(int id, const std::string& username);

    // Permisiuni
    bool addUserToGroup(int entityId, const std::string& username);
    bool removeUserFromGroup(int entityId, const std::string& username);
    bool checkPermission(int entityId, const std::string& username, const std::string& operation);
    bool setPermission(int entityId, const std::string& username, bool canRead, bool canWrite);
    bool updateGroupPermissions(int entityId, bool canRead, bool canWrite);
    bool updateOthersPermissions(int entityId, bool canRead, bool canWrite);
    std::vector<Group> getEntityGroups(int entityId);
    PermissionManager loadPermissions(int entityId);

    // Share & Partajare Recursiva
    bool shareEntity(int entityId, const std::string& username);
    bool revokeShare(int entityId, const std::string& username);
    std::vector<std::string> getSharedWithList(int entityId) const;
    int getParentId(int entityId) const;

    // Utilitare
    int getRootId();
    int getEntityId(const std::string& name, int parentId);
    std::shared_ptr<Folder> buildTree(int folderId, Folder* parent = nullptr,
                                      const std::string& username = "");

private:
    Database& db;
    iLogger& logger;

    bool savePermissions(int entityId, const PermissionManager& pm);
    bool isSharedWith(int entityId, const std::string& username) const;
};