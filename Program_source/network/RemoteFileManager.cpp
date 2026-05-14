#include "RemoteFileManager.h"
#include "../filesystem/Folder.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../permissions/OthersPermission.h"
#include "../permissions/UserPermission.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <sstream>
#include <map>

RemoteFileManager::RemoteFileManager(SocketClient& client)
    : m_client(client) {}

std::string RemoteFileManager::sendCmd(const std::string& cmd) {
    return m_client.sendCommand(cmd);
}

std::vector<std::string> RemoteFileManager::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim))
        result.push_back(token);
    return result;
}

bool RemoteFileManager::initializeDatabase() {
    auto parts = split(sendCmd("FM_INIT"), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::createTextFile(const std::string& name, const std::string& ownerUser,
                                       const std::string& content, int parentId) {
    std::string cmd = "FM_CREATE_TEXT_FILE|" + name + "|" + ownerUser +
                      "|" + std::to_string(parentId) + "|" + content;
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::createFolder(const std::string& name, const std::string& ownerUser,
                                     int parentId) {
    std::string cmd = "FM_CREATE_FOLDER|" + name + "|" + ownerUser +
                      "|" + std::to_string(parentId);
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::createBinaryFile(const std::string& name, const std::string& ownerUser,
                                         const std::string& extension, int parentId) {
    std::string cmd = "FM_CREATE_BINARY_FILE|" + name + "|" + ownerUser +
                      "|" + extension + "|" + std::to_string(parentId);
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::renameEntity(int id, const std::string& newName, const std::string& username) {
    std::string cmd = "FM_RENAME|" + std::to_string(id) + "|" + newName + "|" + username;
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::deleteEntity(int id, const std::string& username) {
    std::string cmd = "FM_DELETE|" + std::to_string(id) + "|" + username;
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::updateTextFile(int id, const std::string& content, const std::string& username) {
    std::string cmd = "FM_UPDATE_TEXT_FILE|" + std::to_string(id) + "|" + username + "|" + content;
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::checkPermission(int entityId, const std::string& username, const std::string& operation) {
    std::string cmd = "FM_CHECK_PERMISSION|" + std::to_string(entityId) + "|" + username + "|" + operation;
    auto parts = split(sendCmd(cmd), '|');
    if (parts.size() >= 2 && parts[0] == "OK") return parts[1] == "1";
    return false;
}

bool RemoteFileManager::checkDirectPermission(int entityId, const std::string& username, const std::string& operation) {
    std::string cmd = "FM_CHECK_DIRECT_PERMISSION|" + std::to_string(entityId) + "|" + username + "|" + operation;
    auto parts = split(sendCmd(cmd), '|');
    if (parts.size() >= 2 && parts[0] == "OK") return parts[1] == "1";
    return false;
}

bool RemoteFileManager::updateOthersPermissions(int entityId, bool canRead, bool canWrite) {
    std::string cmd = "FM_UPDATE_OTHERS_PERM|" + std::to_string(entityId) +
                      "|" + (canRead ? "1" : "0") + "|" + (canWrite ? "1" : "0");
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::shareEntity(int entityId, const std::string& username, bool canRead, bool canWrite) {
    std::string cmd = "FM_SHARE|" + std::to_string(entityId) + "|" + username +
                      "|" + (canRead ? "1" : "0") + "|" + (canWrite ? "1" : "0");
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

bool RemoteFileManager::revokeShare(int entityId, const std::string& username) {
    std::string cmd = "FM_REVOKE_SHARE|" + std::to_string(entityId) + "|" + username;
    auto parts = split(sendCmd(cmd), '|');
    return !parts.empty() && parts[0] == "OK";
}

PermissionManager RemoteFileManager::loadPermissions(int entityId) {
    std::string cmd = "FM_LOAD_PERMISSIONS|" + std::to_string(entityId);
    auto parts = split(sendCmd(cmd), '|');
    PermissionManager pm;
    if (parts.size() >= 3 && parts[0] == "OK") {
        bool oRead  = parts[1] == "1";
        bool oWrite = parts[2] == "1";
        pm.addPermission(std::make_shared<OthersPermission>(oRead, oWrite));
        if (parts.size() >= 4 && !parts[3].empty()) {
            auto up = std::make_shared<UserPermission>();
            up->deserialize(parts[3]);
            pm.addPermission(up);
        }
    }
    return pm;
}

int RemoteFileManager::getRootId() {
    auto parts = split(sendCmd("FM_GET_ROOT_ID"), '|');
    if (parts.size() >= 2 && parts[0] == "OK") return std::stoi(parts[1]);
    return 1;
}

int RemoteFileManager::getParentId(int entityId) const {
    std::string cmd = "FM_GET_PARENT_ID|" + std::to_string(entityId);
    auto parts = split(const_cast<RemoteFileManager*>(this)->sendCmd(cmd), '|');
    if (parts.size() >= 2 && parts[0] == "OK") return std::stoi(parts[1]);
    return 0;
}

std::vector<std::shared_ptr<FileSystemEntity>> RemoteFileManager::getChildren(int parentId, const std::string& username) {
    std::string cmd = "FM_GET_CHILDREN|" + std::to_string(parentId);
    if (!username.empty()) cmd += "|" + username;
    std::string resp = sendCmd(cmd);
    std::cout << "getChildren resp: [" << resp << "]\n";

    size_t sep = resp.find('|');
    if (sep == std::string::npos || resp.substr(0, sep) != "OK") return {};

    std::string jsonStr = resp.substr(sep + 1);
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(jsonStr));
    if (!doc.isArray()) return {};

    std::vector<std::shared_ptr<FileSystemEntity>> children;
    QJsonArray arr = doc.array();

    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        int id           = obj["id"].toInt();
        std::string type = obj["type"].toString().toStdString();
        std::string name = obj["name"].toString().toStdString();
        std::string owner= obj["owner"].toString().toStdString();
        std::time_t created  = (std::time_t)obj["createdAt"].toDouble();
        std::time_t modified = (std::time_t)obj["modifiedAt"].toDouble();
        std::size_t size = static_cast<std::size_t>(obj["size"].toDouble());

        std::shared_ptr<FileSystemEntity> entity;

        if (type == "Folder") {
            auto folder = std::make_shared<Folder>(name, owner, nullptr);
            folder->setId(id);
            folder->setCreatedAt(created);
            folder->setModifiedAt(modified);
            folder->setCachedSize(size);
            entity = folder;
        } else if (type == "TextFile") {
            std::string content = obj["content"].toString().toStdString();
            auto tf = std::make_shared<TextFile>(name, owner, content);
            tf->setId(id);
            tf->setCreatedAt(created);
            tf->setModifiedAt(modified);
            entity = tf;
        } else if (type == "BinaryFile") {
            std::string ext = obj["extension"].toString().toStdString();
            auto bf = std::make_shared<BinaryFile>(name, owner, ext);
            bf->setId(id);
            bf->setCreatedAt(created);
            bf->setModifiedAt(modified);
            bf->setSize(size);
            entity = bf;
        }

        if (entity) children.push_back(entity);
    }

    return children;
}

int RemoteFileManager::getEntityId(const std::string& name, int parentId) {
    std::string cmd = "FM_GET_ENTITY_ID|" + name + "|" + std::to_string(parentId);
    auto parts = split(sendCmd(cmd), '|');
    if (parts.size() >= 2 && parts[0] == "OK") return std::stoi(parts[1]);
    return -1;
}

std::shared_ptr<Folder> RemoteFileManager::buildTree(int folderId, Folder* /*parent*/,
                                                     const std::string& username) {
    std::string cmd = "FM_BUILD_TREE|" + username + "|" + std::to_string(folderId);
    std::string resp = sendCmd(cmd);

    size_t sep = resp.find('|');
    if (sep == std::string::npos || resp.substr(0, sep) != "OK") return nullptr;

    std::string jsonStr = resp.substr(sep + 1);
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(jsonStr));
    if (!doc.isArray()) return nullptr;

    QJsonArray arr = doc.array();
    std::map<int, std::shared_ptr<FileSystemEntity>> entityMap;
    std::map<int, int> parentMap;

    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        int id          = obj["id"].toInt();
        int pid         = obj["parentId"].toInt();
        std::string type = obj["type"].toString().toStdString();
        std::string name = obj["name"].toString().toStdString();
        std::string owner= obj["owner"].toString().toStdString();
        std::time_t created  = (std::time_t)obj["createdAt"].toDouble();
        std::time_t modified = (std::time_t)obj["modifiedAt"].toDouble();
        std::size_t size = static_cast<std::size_t>(obj["size"].toDouble());

        parentMap[id] = pid;

        std::shared_ptr<FileSystemEntity> entity;
        if (type == "Folder") {
            auto folder = std::make_shared<Folder>(name, owner, nullptr);
            folder->setId(id); folder->setCreatedAt(created); folder->setModifiedAt(modified);
            folder->setCachedSize(size);
            entity = folder;
        } else if (type == "TextFile") {
            std::string content = obj["content"].toString().toStdString();
            auto tf = std::make_shared<TextFile>(name, owner, content);
            tf->setId(id); tf->setCreatedAt(created); tf->setModifiedAt(modified);
            entity = tf;
        } else if (type == "BinaryFile") {
            std::string ext = obj["extension"].toString().toStdString();
            auto bf = std::make_shared<BinaryFile>(name, owner, ext);
            bf->setId(id); bf->setCreatedAt(created); bf->setModifiedAt(modified);
            bf->setSize(size);
            entity = bf;
        }
        if (entity) entityMap[id] = entity;
    }

    std::shared_ptr<Folder> root = nullptr;
    for (auto& [id, entity] : entityMap) {
        int pid = parentMap[id];
        if (pid == 0) {
            root = std::dynamic_pointer_cast<Folder>(entity);
        } else {
            auto it = entityMap.find(pid);
            if (it != entityMap.end()) {
                auto parentFolder = std::dynamic_pointer_cast<Folder>(it->second);
                if (parentFolder) {
                    if (entity->isFolder()) {
                        auto cf = std::dynamic_pointer_cast<Folder>(entity);
                        if (cf) cf->setParent(parentFolder.get());
                    }
                    parentFolder->addChild(entity);
                }
            }
        }
    }
    return root;
}
