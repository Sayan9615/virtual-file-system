#include "FileManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include <algorithm>

FileManager::FileManager(Database& db, iLogger& logger)
    : db(db), logger(logger) {}

bool FileManager::initializeDatabase() {
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS entities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            owner_user TEXT NOT NULL,
            parent_id INTEGER,
            created_at INTEGER,
            modified_at INTEGER,
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
        db.execute("INSERT INTO entities(name, type, owner_user, parent_id, created_at, modified_at) "
                   "VALUES('root', 'FOLDER', 'admin', NULL, strftime('%s','now'), strftime('%s','now'));");
    }

    return true;
}

bool FileManager::createTextFile(const std::string& name, const std::string& ownerUser,
                                 const std::string& content, int parentId) {
    if (entityExists(name, parentId))
        return false;

    const char* sql = "INSERT INTO entities(name, type, owner_user, parent_id, created_at, modified_at) "
                      "VALUES(?, 'TEXT', ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, parentId);

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
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "Fisier creat: " + name, logger);
    return true;
}

bool FileManager::createFolder(const std::string& name, const std::string& ownerUser, int parentId) {
    if (entityExists(name, parentId))
        return false;

    const char* sql = "INSERT INTO entities(name, type, owner_user, parent_id, created_at, modified_at) "
                      "VALUES(?, 'FOLDER', ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, parentId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) return false;

    int entityId = sqlite3_last_insert_rowid(db.getConnection());

    PermissionManager pm;
    pm.addPermission(std::make_shared<OwnerPermission>(ownerUser, true, true));
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "Folder creat: " + name, logger);
    return true;
}

std::shared_ptr<TextFile> FileManager::getTextFile(int id) {
    const char* sql = "SELECT e.name, e.owner_user, fc.content, e.created_at, e.modified_at "
                      "FROM entities e JOIN file_contents fc ON e.id = fc.entity_id "
                      "WHERE e.id = ? AND e.type = 'TEXT';";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return nullptr;
    }

    const char* name    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* owner   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    long long created   = sqlite3_column_int64(stmt, 3);
    long long modified  = sqlite3_column_int64(stmt, 4);

    auto file = std::make_shared<TextFile>(
        name    ? name    : "",
        owner   ? owner   : "",
        content ? content : ""
    );
    file->setId(id);
    file->setCreatedAt(created);
    file->setModifiedAt(modified);

    sqlite3_finalize(stmt);
    return file;
}

std::vector<std::shared_ptr<FileSystemEntity>> FileManager::getChildren(int parentId, const std::string& username) {
    std::vector<std::shared_ptr<FileSystemEntity>> children;

    const char* sql = "SELECT id, name, type, owner_user, created_at, modified_at FROM entities WHERE parent_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parentId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id             = sqlite3_column_int(stmt, 0);
        const char* name   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* type   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* owner  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        long long created  = sqlite3_column_int64(stmt, 4);
        long long modified = sqlite3_column_int64(stmt, 5);

        std::string typeStr = type ? type : "";

        if (typeStr == "TEXT") {
            auto tf = getTextFile(id);
            if (tf) children.push_back(tf);
        }
        else if (typeStr == "BINARY") {
            auto [extension, size] = getFileContentInfo(id);
            auto bf = std::make_shared<BinaryFile>(name ? name : "", owner ? owner : "", extension);
            bf->setId(id);
            bf->setCreatedAt(created);
            bf->setModifiedAt(modified);
            bf->setSize(size);
            children.push_back(bf);
        }
        else if (typeStr == "FOLDER") {
            auto folder = std::make_shared<Folder>(name ? name : "", owner ? owner : "");
            folder->setId(id);
            folder->setCreatedAt(created);
            folder->setModifiedAt(modified);
            folder->setCachedSize(getEntitySize(id));
            children.push_back(folder);
        }
    }

    sqlite3_finalize(stmt);

    if (!username.empty()) {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [&](const std::shared_ptr<FileSystemEntity>& e) {
                    return !checkDirectPermission(e->getId(), username, "read");
                }),
            children.end()
        );
    }

    return children;
}

bool FileManager::checkDirectPermission(int entityId, const std::string& username, const std::string& operation) {
    PermissionManager pm = loadPermissions(entityId);
    return pm.check(username, operation);
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

    return deleteEntityTree(id, username);
}

bool FileManager::deleteEntityTree(int id, const std::string& username) {
    if (id <= 1) return false;

    const std::string subtree =
        "WITH RECURSIVE subtree(id) AS ("
        "SELECT id FROM entities WHERE id = " + std::to_string(id) + " "
        "UNION ALL "
        "SELECT e.id FROM entities e JOIN subtree s ON e.parent_id = s.id"
        ") ";

    db.execute("BEGIN TRANSACTION;");
    bool success =
        db.execute(subtree + "DELETE FROM file_contents WHERE entity_id IN (SELECT id FROM subtree);") &&
        db.execute(subtree + "DELETE FROM permissions WHERE entity_id IN (SELECT id FROM subtree);") &&
        db.execute(subtree + "DELETE FROM entities WHERE id IN (SELECT id FROM subtree);");

    db.execute(success ? "COMMIT;" : "ROLLBACK;");

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
    if (sqlite3_step(stmt) == SQLITE_ROW)
        parentId = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return parentId;
}

std::size_t FileManager::getEntitySize(int entityId) const {
    const char* sql = R"(
        WITH RECURSIVE subtree(id) AS (
            SELECT id FROM entities WHERE id = ?
            UNION ALL
            SELECT e.id FROM entities e JOIN subtree s ON e.parent_id = s.id
        )
        SELECT COALESCE(SUM(fc.size), 0)
        FROM subtree s
        JOIN file_contents fc ON fc.entity_id = s.id;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return 0;

    sqlite3_bind_int(stmt, 1, entityId);

    std::size_t size = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_int64 value = sqlite3_column_int64(stmt, 0);
        size = value > 0 ? static_cast<std::size_t>(value) : 0;
    }

    sqlite3_finalize(stmt);
    return size;
}

std::pair<std::string, std::size_t> FileManager::getFileContentInfo(int entityId) const {
    const char* sql = "SELECT extension, size FROM file_contents WHERE entity_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {".bin", 0};

    sqlite3_bind_int(stmt, 1, entityId);

    std::string extension = ".bin";
    std::size_t size = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* ext = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sqlite3_int64 value = sqlite3_column_int64(stmt, 1);
        if (ext && ext[0] != '\0') extension = ext;
        if (value > 0) size = static_cast<std::size_t>(value);
    }

    sqlite3_finalize(stmt);
    return {extension, size};
}

bool FileManager::checkPermission(int entityId, const std::string& username, const std::string& operation) {
    int currentId = entityId;
    while (currentId > 0) {
        if (currentId == 1 && entityId != 1)
            break;

        PermissionManager pm = loadPermissions(currentId);
        if (pm.check(username, operation))
            return true;
        currentId = getParentId(currentId);
    }
    return false;
}

bool FileManager::shareEntity(int entityId, const std::string& username, bool canRead, bool canWrite) {
    // Adauga permisiunea pe entitatea tinta
    PermissionManager pm = loadPermissions(entityId);
    auto up = pm.getUserPermission();
    if (!up) up = std::make_shared<UserPermission>();
    up->addUser(username, canRead, canWrite);
    pm.addPermission(up);
    savePermissions(entityId, pm);

    // Propaga permisiunile pe toti copiii recursiv
    propagateUserPermToChildren(entityId, username, canRead, canWrite);

    // Propaga read-only pe toate folderele parinte pentru navigare
    if (canRead) {
        int currentId = getParentId(entityId);
        while (currentId > 0) {
            PermissionManager parentPm = loadPermissions(currentId);
            auto parentUp = parentPm.getUserPermission();
            if (!parentUp) parentUp = std::make_shared<UserPermission>();
            if (!parentUp->hasUser(username)) {
                parentUp->addUser(username, true, false);
                parentPm.addPermission(parentUp);
                savePermissions(currentId, parentPm);
            }
            currentId = getParentId(currentId);
        }
    }

    EventLog(EventType::FILE_SHARE, username, "Entitate partajata cu: " + username, logger);
    return true;
}

bool FileManager::revokeShare(int entityId, const std::string& username) {
    PermissionManager pm = loadPermissions(entityId);
    auto up = pm.getUserPermission();
    if (!up) return false;
    up->removeUser(username);
    pm.addPermission(up);
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_SHARE, username, "Share revocat pentru: " + username, logger);
    return true;
}

bool FileManager::updateOthersPermissions(int entityId, bool canRead, bool canWrite) {
    PermissionManager pm = loadPermissions(entityId);
    pm.addPermission(std::make_shared<OthersPermission>(canRead, canWrite));
    bool ok = savePermissions(entityId, pm);

    // Propaga permisiunile others pe toti copiii recursiv
    propagateOthersPermToChildren(entityId, canRead, canWrite);

    if (canRead) {
        int currentId = getParentId(entityId);
        while (currentId > 0) {
            PermissionManager parentPm = loadPermissions(currentId);
            auto others = parentPm.getOthersPermission();
            if (!others || !others->canRead()) {
                parentPm.addPermission(std::make_shared<OthersPermission>(true, false));
                savePermissions(currentId, parentPm);
            }
            currentId = getParentId(currentId);
        }
    }

    return ok;
}

std::vector<std::pair<int,bool>> FileManager::getChildIds(int parentId) {
    std::vector<std::pair<int,bool>> result;
    const char* sql = "SELECT id, type FROM entities WHERE parent_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parentId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        bool isFolder = t && std::string(t) == "FOLDER";
        result.push_back({id, isFolder});
    }
    sqlite3_finalize(stmt);
    return result;
}

void FileManager::propagateUserPermToChildren(int folderId, const std::string& username, bool canRead, bool canWrite) {
    for (auto& [childId, isFolder] : getChildIds(folderId)) {
        PermissionManager pm = loadPermissions(childId);
        auto up = pm.getUserPermission();
        if (!up) up = std::make_shared<UserPermission>();
        up->addUser(username, canRead, canWrite);
        pm.addPermission(up);
        savePermissions(childId, pm);
        if (isFolder)
            propagateUserPermToChildren(childId, username, canRead, canWrite);
    }
}

void FileManager::propagateOthersPermToChildren(int folderId, bool canRead, bool canWrite) {
    for (auto& [childId, isFolder] : getChildIds(folderId)) {
        PermissionManager pm = loadPermissions(childId);
        pm.addPermission(std::make_shared<OthersPermission>(canRead, canWrite));
        savePermissions(childId, pm);
        if (isFolder)
            propagateOthersPermToChildren(childId, canRead, canWrite);
    }
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
        sqlite3_bind_int(stmt, 3, 1);
        sqlite3_bind_int(stmt, 4, 1);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    auto up = pm.getUserPermission();
    if (up) {
        const char* sql = "INSERT INTO permissions(entity_id, type, members, can_read, can_write) VALUES(?, 'USER', ?, 0, 0);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, entityId);
        sqlite3_bind_text(stmt, 2, up->serialize().c_str(), -1, SQLITE_TRANSIENT);
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

    const char* sql = "SELECT type, owner_user, members, can_read, can_write FROM permissions WHERE entity_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* type    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        bool canRead        = sqlite3_column_int(stmt, 3);
        bool canWrite       = sqlite3_column_int(stmt, 4);
        std::string typeStr = type ? type : "";

        if (typeStr == "OWNER") {
            const char* ownerUser = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            pm.addPermission(std::make_shared<OwnerPermission>(ownerUser ? ownerUser : "", true, true));
        }
        else if (typeStr == "USER") {
            const char* membersRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            auto up = std::make_shared<UserPermission>();
            if (membersRaw) up->deserialize(membersRaw);
            pm.addPermission(up);
        }
        else if (typeStr == "OTHERS") {
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

bool FileManager::entityExists(const std::string& name, int parentId) const {
    const char* sql = "SELECT 1 FROM entities WHERE name = ? AND parent_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, parentId);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

std::shared_ptr<Folder> FileManager::buildTree(int folderId, Folder* parent, const std::string& username) {
    const char* sql = "SELECT name, owner_user, created_at, modified_at FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, folderId);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return nullptr;
    }

    std::string name   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string owner  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    long long created  = sqlite3_column_int64(stmt, 2);
    long long modified = sqlite3_column_int64(stmt, 3);
    sqlite3_finalize(stmt);

    auto folder = std::make_shared<Folder>(name, owner, parent);
    folder->setId(folderId);

    const char* childSql = "SELECT id, name, type, owner_user FROM entities WHERE parent_id = ?;";
    sqlite3_prepare_v2(db.getConnection(), childSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, folderId);

    std::vector<int> subfolderIds;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id            = sqlite3_column_int(stmt, 0);
        const char* cname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* ctype = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* cown  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string typeStr = ctype ? ctype : "";

        if (!username.empty() && !checkDirectPermission(id, username, "read"))
            continue;

        if (typeStr == "TEXT") {
            auto tf = getTextFile(id);
            if (tf) {
                try { folder->addChild(tf); } catch (...) {}
            }
        }
        else if (typeStr == "BINARY") {
            auto [extension, size] = getFileContentInfo(id);
            auto bf = std::make_shared<BinaryFile>(cname ? cname : "", cown ? cown : "", extension);
            bf->setId(id);
            bf->setSize(size);
            try { folder->addChild(bf); } catch (...) {}
        }
        else if (typeStr == "FOLDER") {
            subfolderIds.push_back(id);
        }
    }
    sqlite3_finalize(stmt);

    for (int id : subfolderIds) {
        auto sub = buildTree(id, folder.get(), username);
        if (sub) {
            try { folder->addChild(sub); } catch (...) {}
        }
    }

    folder->setCreatedAt(created);
    folder->setModifiedAt(modified);

    return folder;
}

bool FileManager::createBinaryFile(const std::string& name, const std::string& ownerUser,
                                   const std::string& extension, int parentId) {
    if (entityExists(name, parentId))
        return false;

    const char* sql = "INSERT INTO entities(name, type, owner_user, parent_id, created_at, modified_at) "
                      "VALUES(?, 'BINARY', ?, ?, strftime('%s','now'), strftime('%s','now'));";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerUser.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, parentId);

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
    savePermissions(entityId, pm);

    EventLog(EventType::FILE_UPLOAD, ownerUser, "BinaryFile creat: " + name + extension, logger);
    return true;
}
