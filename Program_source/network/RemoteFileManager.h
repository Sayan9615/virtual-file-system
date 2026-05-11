#pragma once
#include "../interfaces/IFileManager.h"
#include "SocketClient.h"
#include <string>
#include <vector>
#include <memory>

class RemoteFileManager : public IFileManager {
public:
    explicit RemoteFileManager(SocketClient& client);

    bool initializeDatabase() override;
    bool createTextFile(const std::string& name, const std::string& ownerUser,
                        const std::string& ownerGroup, const std::string& content,
                        int parentId = 1) override;
    bool createFolder(const std::string& name, const std::string& ownerUser,
                      const std::string& ownerGroup, int parentId = 1) override;
    bool renameEntity(int id, const std::string& newName, const std::string& username) override;
    bool deleteEntity(int id, const std::string& username) override;


    bool updateTextFile(int id, const std::string& content, const std::string& username) override;

    bool checkPermission(int entityId, const std::string& username, const std::string& operation) override;
    bool setPermission(int entityId, const std::string& username, bool canRead, bool canWrite) override;
    bool updateGroupPermissions(int entityId, bool canRead, bool canWrite) override;
    bool updateOthersPermissions(int entityId, bool canRead, bool canWrite) override;
    bool shareEntity(int entityId, const std::string& username) override;
    bool revokeShare(int entityId, const std::string& username) override;
    std::vector<std::string> getSharedWithList(int entityId) override;
    bool addUserToGroup(int entityId, const std::string& username) override;
    bool removeUserFromGroup(int entityId, const std::string& username) override;
    PermissionManager loadPermissions(int entityId) override;
    int getRootId() override;
    std::shared_ptr<Folder> buildTree(int folderId, Folder* parent = nullptr,
                                      const std::string& username = "") override;

    bool createBinaryFile(const std::string& name, const std::string& ownerUser,
                          const std::string& ownerGroup, const std::string& extension,
                          int parentId = 1) override;
    int getEntityId(const std::string& name, int parentId) override;
    std::vector<std::shared_ptr<FileSystemEntity>> getChildren(int parentId) override;

private:
    SocketClient& m_client;
    std::string sendCmd(const std::string& cmd);
    static std::vector<std::string> split(const std::string& s, char delim);
};