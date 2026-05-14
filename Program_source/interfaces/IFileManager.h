#pragma once
#include <string>
#include <vector>
#include <memory>
#include "PermissionManager.h"

class Folder;
class FileSystemEntity;

class IFileManager {
public:
    virtual ~IFileManager() = default;

    virtual bool initializeDatabase() = 0;

    // ── Creare ───────────────────────────────────────
    virtual bool createTextFile(const std::string& name,
                                const std::string& ownerUser,
                                const std::string& content,
                                int parentId = 1) = 0;

    virtual bool createBinaryFile(const std::string& name,
                                  const std::string& ownerUser,
                                  const std::string& extension,
                                  int parentId = 1) = 0;

    virtual bool createFolder(const std::string& name,
                              const std::string& ownerUser,
                              int parentId = 1) = 0;

    // ── Modificare ───────────────────────────────────
    virtual bool renameEntity(int id,
                              const std::string& newName,
                              const std::string& username) = 0;

    virtual bool deleteEntity(int id,
                              const std::string& username) = 0;

    virtual bool updateTextFile(int id,
                                const std::string& content,
                                const std::string& username) = 0;

    // ── Permisiuni ───────────────────────────────────
    virtual bool checkPermission(int entityId,
                                 const std::string& username,
                                 const std::string& operation) = 0;

    virtual bool checkDirectPermission(int entityId,
                                       const std::string& username,
                                       const std::string& operation) = 0;

    virtual bool updateOthersPermissions(int entityId,
                                         bool canRead,
                                         bool canWrite) = 0;

    virtual PermissionManager loadPermissions(int entityId) = 0;

    // ── Partajare ────────────────────────────────────
    virtual bool shareEntity(int entityId,
                             const std::string& username,
                             bool canRead,
                             bool canWrite) = 0;

    virtual bool revokeShare(int entityId,
                             const std::string& username) = 0;

    // ── Navigare / Arbore ────────────────────────────
    virtual int getRootId() = 0;

    virtual int getEntityId(const std::string& name,
                            int parentId) = 0;

    virtual std::vector<std::shared_ptr<FileSystemEntity>>
    getChildren(int parentId, const std::string& username = "") = 0;

    virtual std::shared_ptr<Folder> buildTree(
        int folderId,
        Folder* parent = nullptr,
        const std::string& username = "") = 0;

    virtual int getParentId(int entityId) const = 0;
};