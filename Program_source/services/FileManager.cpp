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

    // Creaza root daca nu exista
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

    // Salveaza continutul
    const char* contentSql = "INSERT INTO file_contents(entity_id, extension, content, size) VALUES(?, '.txt', ?, ?);";
    sqlite3_prepare_v2(db.getConnection(), contentSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);
    sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, content.size());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Salveaza permisiuni default
    PermissionManager pm;
    pm.addPermission(std::make_shared<OwnerPermission>(ownerUser, true, true));
    pm.addPermission(std::make_shared<GroupPermission>(ownerGroup, true, false));
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
    pm.addPermission(std::make_shared<GroupPermission>(ownerGroup, true, false));
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
    const char* sql = "SELECT e.name, e.owner_user, e.owner_group, fc.content "
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

    auto file = std::make_shared<TextFile>(
        name ? name : "",
        owner ? owner : "",
        group ? group : "",
        content ? content : ""
    );

    sqlite3_finalize(stmt);
    return file;
}

std::vector<std::shared_ptr<FileSystemEntity>> FileManager::getChildren(int parentId) {
    std::vector<std::shared_ptr<FileSystemEntity>> children;

    const char* sql = "SELECT id, name, type, owner_user, owner_group FROM entities WHERE parent_id = ?;";
    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, parentId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* group = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        std::string typeStr = type ? type : "";

        if (typeStr == "TEXT") {
            children.push_back(getTextFile(id));
        } else if (typeStr == "FOLDER") {
            children.push_back(std::make_shared<Folder>(
                name ? name : "", owner ? owner : "", group ? group : ""));
        } else if (typeStr == "SHARED_FOLDER") {
            children.push_back(std::make_shared<SharedFolder>(
                name ? name : "", owner ? owner : "", group ? group : ""));
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

bool FileManager::shareEntity(int entityId, const std::string& username) {
    // Citeste shared_with curent
    const char* getSql = "SELECT shared_with FROM entities WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db.getConnection(), getSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, entityId);

    std::string sharedWith = "";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* sw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sharedWith = sw ? sw : "";
    }
    sqlite3_finalize(stmt);

    if (!sharedWith.empty()) sharedWith += ",";
    sharedWith += username;

    const char* updateSql = "UPDATE entities SET shared_with = ? WHERE id = ?;";
    sqlite3_prepare_v2(db.getConnection(), updateSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, sharedWith.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, entityId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        EventLog(EventType::FILE_SHARE, username, "Entitate partajata cu: " + username, logger);

    return success;
}

bool FileManager::checkPermission(int entityId, const std::string& username, const std::string& operation) {
    PermissionManager pm = loadPermissions(entityId);
    return pm.check(username, operation);
}

bool FileManager::savePermissions(int entityId, const PermissionManager& pm) {
    // Sterge permisiunile vechi
    db.execute("DELETE FROM permissions WHERE entity_id = " + std::to_string(entityId) + ";");

    // Salveaza owner
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

    // Salveaza group
    auto group = pm.getGroupPermission();
    if (group) {
        std::string members = "";
        for (const auto& m : group->getMembers()) members += m + ",";

        const char* sql = "INSERT INTO permissions(entity_id, type, group_name, members, can_read, can_write) VALUES(?, 'GROUP', ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db.getConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, entityId);
        sqlite3_bind_text(stmt, 2, group->getGroupName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, members.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, group->canRead() ? 1 : 0);
        sqlite3_bind_int(stmt, 5, group->canWrite() ? 1 : 0);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Salveaza others
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
            const char* groupName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* members = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            auto group = std::make_shared<GroupPermission>(groupName ? groupName : "", canRead, canWrite);

            if (members) {
                std::istringstream ss(members);
                std::string member;
                while (std::getline(ss, member, ','))
                    if (!member.empty()) group->addMember(member);
            }
            pm.addPermission(group);
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