#include "FileManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

FileManager::FileManager(Database& db, iLogger& logger)
    : db(db), logger(logger) {}

bool FileManager::initializeDatabase() {
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS entities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            owner_user TEXT NOT NULL,
            owner_group TEXT NOT NULL,
            parent_id INTEGER,
            created_at INTEGER,
            modified_at INTEGER,
            shared_with TEXT DEFAULT '',
            is_public INTEGER DEFAULT 0,
            FOREIGN KEY (parent_id) REFERENCES entities(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS file_contents (
            entity_id INTEGER PRIMARY KEY,
            extension TEXT,
            content TEXT,
            size INTEGER,
            FOREIGN KEY (entity_id) REFERENCES entities(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS permissions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            entity_id INTEGER,
            type TEXT,
            owner_user TEXT,
            group_name TEXT,
            members TEXT,
            can_read INTEGER,
            can_write INTEGER,
            FOREIGN KEY (entity_id) REFERENCES entities(id) ON DELETE CASCADE
        );
    )";

    if (!db.execute(sql)) return false;

    const char* checkRoot = "SELECT id FROM entities WHERE id = 1;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), checkRoot, -1, &stmt, nullptr);
    bool rootExists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    if (!rootExists) {
        db.execute("INSERT INTO entities(name, type, owner_user, owner_group, parent_id, created_at, modified_at) "
                   "VALUES('root', 'FOLDER', 'admin', 'admin', NULL, strftime('%s','now'), strftime('%s','now'));");
    }

    return true;
}

bool FileManager::createTextFile(const std::string& name, const std::string& ownerUser,
                                 const std::string& ownerGroup, const std::string& content,
                                 int parentId) {
    const char* sql = "INSERT INTO entities(name, type, owner_user, owner_group, parent_id, created_at, modified_at) "
                      "VALUES(?, 'TEXT', ?, ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ownerGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, parentId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) return false;

    int entityId = sqlite3_last_insert_rowid(db.getConnection());

    const char* contentSql = "INSERT INTO file_contents(entity_id, extension, content, size) VALUES(?, '.txt', ?, ?);";
    sqlite3_prepare_v2(db.getConnection(), contentSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);
    sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, content.size());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    PermissionManager pm;
    pm.addPermission(std::make_shared<OwnerPermission>(ownerUser, true, true));
    {
        Group g(ownerGroup);
        g.addMember(ownerUser);
        auto gp = std::make_shared<GroupPermission>(true, false);
        gp->addGroup(g);
        pm.addPermission(gp);
    }
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "Fisier creat: " + name, logger);
    return true;
}

bool FileManager::createFolder(const std::string& name, const std::string& ownerUser,
                               const std::string& ownerGroup, int parentId) {
    const char* sql = "INSERT INTO entities(name, type, owner_user, owner_group, parent_id, created_at, modified_at) "
                      "VALUES(?, 'FOLDER', ?, ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ownerGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, parentId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) return false;

    int entityId = sqlite3_last_insert_rowid(db.getConnection());

    PermissionManager pm;
    pm.addPermission(std::make_shared<OwnerPermission>(ownerUser, true, true));
    {
        Group g(ownerGroup);
        g.addMember(ownerUser);
        auto gp = std::make_shared<GroupPermission>(true, false);
        gp->addGroup(g);
        pm.addPermission(gp);
    }
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "Folder creat: " + name, logger);
    return true;
}

bool FileManager::createSharedFolder(const std::string& name, const std::string& ownerUser,
                                     const std::string& ownerGroup, bool isPublic, int parentId) {
    const char* sql = "INSERT INTO entities(name, type, owner_user, owner_group, parent_id, is_public, created_at, modified_at) "
                      "VALUES(?, 'SHARED_FOLDER', ?, ?, ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ownerGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, parentId);
    sqlite3_bind_int(stmt, 5, isPublic ? 1 : 0);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_SHARE, ownerUser, "SharedFolder creat: " + name, logger);

    return success;
}

std::shared_ptr<TextFile> FileManager::getTextFile(int id) {
    const char* sql = "SELECT e.name, e.owner_user, e.owner_group, fc.content, e.created_at, e.modified_at "
                      "FROM entities e JOIN file_contents fc ON e.id = fc.entity_id "
                      "WHERE e.id = ? AND e.type = 'TEXT';";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return nullptr;
    }

    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* group = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    long long created = sqlite3_column_int64(stmt, 4);
    long long modified = sqlite3_column_int64(stmt, 5);

    auto file = std::make_shared<TextFile>(
        name ? name : "",
        owner ? owner : "",
        group ? group : "",
        content ? content : ""
        );
    file->setId(id);
    file->setCreatedAt(created);
    file->setModifiedAt(modified);

    sqlite3_finalize(stmt);
    return file;
}

std::vector<std::shared_ptr<FileSystemEntity>> FileManager::getChildren(int parentId) {
    std::vector<std::shared_ptr<FileSystemEntity>> children;

    const char* sql = "SELECT id, name, type, owner_user, owner_group, created_at, modified_at FROM entities WHERE parent_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parentId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* group = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        long long created = sqlite3_column_int64(stmt, 5);
        long long modified = sqlite3_column_int64(stmt, 6);

        std::string typeStr = type ? type : "";

        if (typeStr == "TEXT") {
            auto tf = getTextFile(id);
            if (tf) children.push_back(tf);
        } else if (typeStr == "FOLDER") {
            auto folder = std::make_shared<Folder>(name ? name : "", owner ? owner : "", group ? group : "");
            folder->setId(id);
            folder->setCreatedAt(created);
            folder->setModifiedAt(modified);
            children.push_back(folder);
        } else if (typeStr == "SHARED_FOLDER") {
            auto sFolder = std::make_shared<SharedFolder>(name ? name : "", owner ? owner : "", group ? group : "");
            sFolder->setId(id);
            sFolder->setCreatedAt(created);
            sFolder->setModifiedAt(modified);
            children.push_back(sFolder);
        }
    }

    sqlite3_finalize(stmt);
    return children;
}

bool FileManager::updateTextFile(int id, const std::string& content, const std::string& username) {
    if (!checkPermission(id, username, "write")) {
        EventLog(EventType::PERMISSION_DENIED, username, "Write refuzat pentru entity id=" + std::to_string(id), logger);
        return false;
    }

    const char* sql = "UPDATE file_contents SET content = ?, size = ? WHERE entity_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, content.size());
    sqlite3_bind_int(stmt, 3, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_UPLOAD, username, "Fisier actualizat id=" + std::to_string(id), logger);

    return success;
}

bool FileManager::renameEntity(int id, const std::string& newName, const std::string& username) {
    if (!checkPermission(id, username, "write")) {
        EventLog(EventType::PERMISSION_DENIED, username, "Redenumire refuzata pentru entity id=" + std::to_string(id), logger);
        return false;
    }

    const char* sql = "UPDATE entities SET name = ?, modified_at = strftime('%s','now') WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_UPLOAD, username, "Entitate redenumita id=" + std::to_string(id) + " in " + newName, logger);

    return success;
}

bool FileManager::deleteEntity(int id, const std::string& username) {
    if (!checkPermission(id, username, "write")) {
        EventLog(EventType::PERMISSION_DENIED, username, "Delete refuzat pentru entity id=" + std::to_string(id), logger);
        return false;
    }

    const char* sql = "DELETE FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_DELETE, username, "Entitate stearsa id=" + std::to_string(id), logger);

    return success;
}

int FileManager::getParentId(int entityId) const {
    const char* sql = "SELECT parent_id FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);

    int parentId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        parentId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return parentId;
}

std::vector<std::string> FileManager::getSharedWithList(int entityId) const {
    const char* sql = "SELECT shared_with FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::vector<std::string> users;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* sw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (sw) {
            std::istringstream ss(sw);
            std::string token;
            while (std::getline(ss, token, ',')) {
                if (!token.empty()) users.push_back(token);
            }
        }
    }
    sqlite3_finalize(stmt);
    return users;
}

bool FileManager::shareEntity(int entityId, const std::string& username) {
    auto users = getSharedWithList(entityId);
    std::string sharedWith = "";
    for(const auto& u : users) {
        if(u != username) sharedWith += u + ",";
    }
    sharedWith += username;

    const char* updateSql = "UPDATE entities SET shared_with = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), updateSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, sharedWith.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, entityId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_SHARE, username, "Entitate partajata cu: " + username, logger);

    return success;
}

bool FileManager::revokeShare(int entityId, const std::string& username) {
    auto users = getSharedWithList(entityId);
    std::string newShared = "";
    bool first = true;
    for(const auto& u : users) {
        if (u != username) {
            if (!first) newShared += ",";
            newShared += u;
            first = false;
        }
    }

    const char* updateSql = "UPDATE entities SET shared_with = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), updateSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, newShared.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, entityId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_SHARE, username, "Share revocat pentru: " + username, logger);

    return success;
}

bool FileManager::isSharedWith(int entityId, const std::string& username) const {
    int currentId = entityId;
    while (currentId > 0) {
        auto users = getSharedWithList(currentId);
        if (std::find(users.begin(), users.end(), username) != users.end()) {
            return true;
        }
        currentId = getParentId(currentId);
    }
    return false;
}

bool FileManager::checkPermission(int entityId, const std::string& username, const std::string& operation) {
    int currentId = entityId;
    while (currentId > 0) {
        if (operation == "read") {
            auto users = getSharedWithList(currentId);
            if (std::find(users.begin(), users.end(), username) != users.end())
                return true;
        }
        PermissionManager pm = loadPermissions(currentId);
        if (pm.check(username, operation))
            return true;
        currentId = getParentId(currentId);
    }
    return false;
}

bool FileManager::addUserToGroup(int entityId, const std::string& username) {
    PermissionManager pm = loadPermissions(entityId);
    auto gp = pm.getGroupPermission();
    if (!gp || gp->getGroups().empty()) return false;

    auto newGp = std::make_shared<GroupPermission>(gp->canRead(), gp->canWrite());
    for (const auto& oldGroup : gp->getGroups()) {
        Group updated = oldGroup;
        updated.addMember(username);
        newGp->addGroup(updated);
    }
    pm.addPermission(newGp);
    savePermissions(entityId, pm);
    return true;
}

bool FileManager::removeUserFromGroup(int entityId, const std::string& username) {
    PermissionManager pm = loadPermissions(entityId);
    auto gp = pm.getGroupPermission();
    if (!gp || gp->getGroups().empty()) return false;

    auto newGp = std::make_shared<GroupPermission>(gp->canRead(), gp->canWrite());
    for (const auto& oldGroup : gp->getGroups()) {
        Group updated = oldGroup;
        updated.removeMember(username);
        newGp->addGroup(updated);
    }
    pm.addPermission(newGp);
    return savePermissions(entityId, pm);
}

bool FileManager::updateGroupPermissions(int entityId, bool canRead, bool canWrite) {
    PermissionManager pm = loadPermissions(entityId);
    auto gp = pm.getGroupPermission();
    if (!gp) {
        gp = std::make_shared<GroupPermission>(canRead, canWrite);
    } else {
        auto newGp = std::make_shared<GroupPermission>(canRead, canWrite);
        for (const auto& g : gp->getGroups()) newGp->addGroup(g);
        gp = newGp;
    }
    pm.addPermission(gp);
    return savePermissions(entityId, pm);
}

bool FileManager::updateOthersPermissions(int entityId, bool canRead, bool canWrite) {
    PermissionManager pm = loadPermissions(entityId);
    pm.addPermission(std::make_shared<OthersPermission>(canRead, canWrite));
    return savePermissions(entityId, pm);
}

bool FileManager::setPermission(int entityId, const std::string& username, bool canRead, bool canWrite) {
    return updateGroupPermissions(entityId, canRead, canWrite);
}

std::vector<Group> FileManager::getEntityGroups(int entityId) {
    auto gp = loadPermissions(entityId).getGroupPermission();
    if (!gp) return {};
    return gp->getGroups();
}

bool FileManager::savePermissions(int entityId, const PermissionManager& pm) {
    db.execute("DELETE FROM permissions WHERE entity_id = " + std::to_string(entityId) + ";");

    auto owner = pm.getOwnerPermission();
    if (owner) {
        const char* sql = "INSERT INTO permissions(entity_id, type, owner_user, can_read, can_write) VALUES(?, 'OWNER', ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, entityId);
        sqlite3_bind_text(stmt, 2, owner->getOwnerUsername().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, owner->canRead() ? 1 : 0);
        sqlite3_bind_int(stmt, 4, owner->canWrite() ? 1 : 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    auto group = pm.getGroupPermission();
    if (group) {
        const auto& groups = group->getGroups();
        std::string serializedGroups;
        for (size_t i = 0; i < groups.size(); ++i) {
            if (i > 0) serializedGroups += ";";
            serializedGroups += groups[i].serialize();
        }
        std::string firstGroupName = groups.empty() ? "" : groups[0].getName();

        const char* sql = "INSERT INTO permissions(entity_id, type, group_name, members, can_read, can_write) VALUES(?, 'GROUP', ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, entityId);
        sqlite3_bind_text(stmt, 2, firstGroupName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, serializedGroups.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, group->canRead() ? 1 : 0);
        sqlite3_bind_int(stmt, 5, group->canWrite() ? 1 : 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    auto others = pm.getOthersPermission();
    if (others) {
        const char* sql = "INSERT INTO permissions(entity_id, type, can_read, can_write) VALUES(?, 'OTHERS', ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, entityId);
        sqlite3_bind_int(stmt, 2, others->canRead() ? 1 : 0);
        sqlite3_bind_int(stmt, 3, others->canWrite() ? 1 : 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return true;
}

PermissionManager FileManager::loadPermissions(int entityId) {
    PermissionManager pm;

    const char* sql = "SELECT type, owner_user, group_name, members, can_read, can_write FROM permissions WHERE entity_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        bool canRead = sqlite3_column_int(stmt, 4);
        bool canWrite = sqlite3_column_int(stmt, 5);
        std::string typeStr = type ? type : "";

        if (typeStr == "OWNER") {
            const char* ownerUser = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            pm.addPermission(std::make_shared<OwnerPermission>(ownerUser ? ownerUser : "", canRead, canWrite));
        } else if (typeStr == "GROUP") {
            const char* groupNameRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* membersRaw   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            auto gp = std::make_shared<GroupPermission>(canRead, canWrite);

            std::string membersStr   = membersRaw   ? membersRaw   : "";
            std::string groupNameStr = groupNameRaw ? groupNameRaw : "";

            if (membersStr.find(':') != std::string::npos) {
                std::istringstream ss(membersStr);
                std::string groupData;
                while (std::getline(ss, groupData, ';')) {
                    if (groupData.empty()) continue;
                    Group g;
                    g.deserialize(groupData);
                    gp->addGroup(g);
                }
            } else {
                Group g(groupNameStr);
                std::istringstream ss(membersStr);
                std::string member;
                while (std::getline(ss, member, ','))
                    if (!member.empty()) g.addMember(member);
                gp->addGroup(g);
            }
            pm.addPermission(gp);
        } else if (typeStr == "OTHERS") {
            pm.addPermission(std::make_shared<OthersPermission>(canRead, canWrite));
        }
    }

    sqlite3_finalize(stmt);
    return pm;
}

int FileManager::getRootId() {
    return 1;
}

int FileManager::getEntityId(const std::string& name, int parentId) {
    const char* sql = "SELECT id FROM entities WHERE name = ? AND parent_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, parentId);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return id;
}

std::shared_ptr<Folder> FileManager::buildTree(int folderId, Folder* parent, const std::string& username) {
    const char* sql = "SELECT name, owner_user, owner_group, created_at, modified_at FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, folderId);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return nullptr;
    }

    std::string name  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    std::string group = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    long long created = sqlite3_column_int64(stmt, 3);
    long long modified = sqlite3_column_int64(stmt, 4);
    sqlite3_finalize(stmt);

    auto folder = std::make_shared<Folder>(name, owner, group, parent);
    folder->setId(folderId);

    const char* childSql = "SELECT id, name, type, owner_user, owner_group FROM entities WHERE parent_id = ?;";
    sqlite3_prepare_v2(db.getConnection(), childSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, folderId);

    std::vector<std::pair<int,std::string>> subfolders;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id       = sqlite3_column_int(stmt, 0);
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (!username.empty() && !checkPermission(id, username, "read"))
            continue;

        // SCUT ANTI-CRASH AICI: Prindem erorile ca sa nu ne explodeze programul
        if (type == "TEXT") {
            auto tf = getTextFile(id);
            if (tf) {
                try { folder->addChild(tf); } catch (...) {}
            }
        } else if (type == "FOLDER" || type == "SHARED_FOLDER") {
            subfolders.push_back({id, type});
        }
    }
    sqlite3_finalize(stmt);

    for (auto& [id, type] : subfolders) {
        auto sub = buildTree(id, folder.get(), username);
        if (sub) {
            try { folder->addChild(sub); } catch (...) {}
        }
    }

    if (parent == nullptr && !username.empty()) {
        const char* sharedSql = "SELECT id, type FROM entities WHERE ',' || shared_with || ',' LIKE ?;";
        std::string pattern = "%," + username + ",%";
        sqlite3_stmt* sharedStmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sharedSql, -1, &sharedStmt, nullptr);
        sqlite3_bind_text(sharedStmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(sharedStmt) == SQLITE_ROW) {
            int sharedId = sqlite3_column_int(sharedStmt, 0);

            // ANTI-BUCLA DE CRASH AICI: Nu incerca sa randezi "Root"-ul inca o data daca a primit share
            if (sharedId == folderId || sharedId == 1) continue;

            std::string type = reinterpret_cast<const char*>(sqlite3_column_text(sharedStmt, 1));

            bool alreadyVisible = false;
            for (const auto& child : folder->getChildren()) {
                if (child->getId() == sharedId) { alreadyVisible = true; break; }
            }

            if (!alreadyVisible) {
                if (type == "TEXT") {
                    auto file = getTextFile(sharedId);
                    if (file) {
                        try { folder->addChild(file); }
                        catch (...) {
                            try {
                                file->setName(file->getName() + "_Shared");
                                folder->addChild(file);
                            } catch (...) {}
                        }
                    }
                } else if (type == "FOLDER" || type == "SHARED_FOLDER") {
                    auto subf = buildTree(sharedId, folder.get(), username);
                    if (subf) {
                        try { folder->addChild(subf); }
                        catch (...) {
                            try {
                                subf->setName(subf->getName() + "_Shared");
                                folder->addChild(subf);
                            } catch (...) {}
                        }
                    }
                }
            }
        }
        sqlite3_finalize(sharedStmt);
    }

    folder->setCreatedAt(created);
    folder->setModifiedAt(modified);

    return folder;
}

bool FileManager::createBinaryFile(const std::string& name, const std::string& ownerUser,
                                   const std::string& ownerGroup, const std::string& extension,
                                   int parentId) {
    const char* sql = "INSERT INTO entities(name, type, owner_user, owner_group, parent_id, created_at, modified_at) "
                      "VALUES(?, 'BINARY', ?, ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ownerGroup.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  4, parentId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) return false;

    int entityId = sqlite3_last_insert_rowid(db.getConnection());

    const char* contentSql = "INSERT INTO file_contents(entity_id, extension, content, size) VALUES(?, ?, '', 0);";
    sqlite3_prepare_v2(db.getConnection(), contentSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt,  1, entityId);
    sqlite3_bind_text(stmt, 2, extension.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    PermissionManager pm;
    pm.addPermission(std::make_shared<OwnerPermission>(ownerUser, true, true));
    {
        Group g(ownerGroup);
        g.addMember(ownerUser);
        auto gp = std::make_shared<GroupPermission>(true, false);
        gp->addGroup(g);
        pm.addPermission(gp);
    }
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "BinaryFile creat: " + name + extension, logger);
    return true;
}