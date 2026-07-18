#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "services/AuthService.h"
#include "services/Database.h"
#include "services/FileManager.h"
#include "logger/ConsoleLogger.h"
#include "filesystem/Folder.h"
#include "filesystem/TextFile.h"
#include "filesystem/BinaryFile.h"


using namespace std;

mutex g_mutex;
Database g_db;
ConsoleLogger g_logger;
AuthService* g_auth = nullptr;
FileManager* g_fm   = nullptr;
unsigned int MAX_MESSAGE_SIZE = 99999999;


static bool sendMsg(SOCKET sock, const string& msg) {
    char lenBuf[9];
    string msg_size = to_string(msg.size());
    int cntr;
    for(cntr = 0;cntr < (8-msg_size.size());cntr++){
        lenBuf[cntr] = '0';
    }
    for(int i = 0;cntr < 8; cntr++){
        lenBuf[cntr] = msg_size[i++];
    }
    lenBuf[8] = '\0';
    //cout << "\n" << lenBuf << "\n";
    int headerSent = 0;
    while (headerSent < 8) {
        int sent = send(sock, lenBuf + headerSent, 8 - headerSent, 0);
        if (sent <= 0) return false;
        headerSent += sent;
    }

    unsigned int total = 0;
    while (total < msg.size()) {
        int n = send(sock, msg.c_str() + total, (int)(msg.size() - total), 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

static string recvMsg(SOCKET sock) {
    char lenBuf[9] = {0};
    int total = 0;
    while (total < 8) {
        int n = recv(sock, lenBuf + total, 8 - total, 0);
        if (n <= 0) return "";
        total += n;
    }
    unsigned int msgLen = atoi(lenBuf);
    //cout << "\n" << lenBuf <<" la recive\n";
    if (msgLen == 0) return "";
    if (msgLen > MAX_MESSAGE_SIZE) return "";

    string result(msgLen, '\0');
    size_t received = 0;
    while (received < msgLen) {
        int n = recv(sock, &result[received], (int)(msgLen - received), 0);
        if (n <= 0) return "";
        received += n;
    }
    //cout << result;
    return result;
}

static vector<string> splitMsg(const string& s, char delim) {
    vector<string> parts;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delim))
        parts.push_back(token);
    return parts;
}

static string jsonEscapeStr(const string& s) {
    string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if      (c == '\\') out += "\\\\";
        else if (c == '"')  out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04X", (unsigned int)c);
            out += buf;
        } else {
            out += (char)c;
        }
    }
    return out;
}

static string serializeEntityToJson(FileSystemEntity* entity, int parentId) {
    string json = "{";
    json += "\"id\":" + to_string(entity->getId()) + ",";

    string typeStr;
    if (dynamic_cast<Folder*>(entity))          typeStr = "Folder";
    else if (dynamic_cast<TextFile*>(entity))   typeStr = "TextFile";
    else if (dynamic_cast<BinaryFile*>(entity)) typeStr = "BinaryFile";
    else                                        typeStr = "Unknown";

    json += "\"type\":\"" + typeStr + "\",";
    json += "\"name\":\"" + jsonEscapeStr(entity->getName()) + "\",";
    json += "\"owner\":\"" + jsonEscapeStr(entity->getOwnerUser()) + "\",";
    json += "\"parentId\":" + to_string(parentId) + ",";
    json += "\"createdAt\":" + to_string((long long)entity->getCreatedAt()) + ",";
    json += "\"modifiedAt\":" + to_string((long long)entity->getModifiedAt()) + ",";
    json += "\"size\":" + to_string((unsigned long long)entity->getSize());

    if (auto* tf = dynamic_cast<TextFile*>(entity)) {
        json += ",\"content\":\"" + jsonEscapeStr(tf->read()) + "\"";
    } else if (auto* bf = dynamic_cast<BinaryFile*>(entity)) {
        json += ",\"extension\":\"" + jsonEscapeStr(bf->getExtension()) + "\"";
    }

    json += "}";
    return json;
}

static void serializeTreeDFS(Folder* folder, int parentId, string& out, bool& first) {
    if (!first) out += ",";
    first = false;
    out += serializeEntityToJson(folder, parentId);
    for (auto& child : folder->getChildren()) {
        FileSystemEntity* entity = child.get();
        if (entity->isFolder()) {
            auto* childFolder = dynamic_cast<Folder*>(entity);
            if (childFolder) serializeTreeDFS(childFolder, folder->getId(), out, first);
        } else {
            out += ",";
            out += serializeEntityToJson(entity, folder->getId());
        }
    }
}

static string serializeTreeToJson(Folder* folder) {
    string json = "[";
    bool first = true;
    serializeTreeDFS(folder, 0, json, first);
    json += "]";
    return json;
}

// Serializeaza copiii directi ai unui folder ca JSON array
static string serializeChildrenToJson(const vector<shared_ptr<FileSystemEntity>>& children, int parentId) {
    string json = "[";
    bool first = true;
    for (const auto& child : children) {
        if (!first) json += ",";
        first = false;
        json += serializeEntityToJson(child.get(), parentId);
    }
    json += "]";
    return json;
}

void handleClient(SOCKET clientSocket) {
    cout << "Client nou conectat.\n";

    while (true) {
        string msg = recvMsg(clientSocket);
        if (msg.empty()) break;

        cout << "Primit: " << msg << "\n";

        auto parts = splitMsg(msg, '|');
        if (parts.empty()) { sendMsg(clientSocket, "ERR|empty command"); continue; }

        string cmd = parts[0];
        string response;

        try {
            lock_guard<mutex> lock(g_mutex);

            // ── Auth commands ──────────────────────────────────────
            if (cmd == "AUTH_REGISTER") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->registerUser(parts[1], parts[2]);
                    response = ok ? "OK" : "ERR|registration failed";
                }
            }
            else if (cmd == "AUTH_LOGIN") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->login(parts[1], parts[2]);
                    response = ok ? "OK" : "ERR|invalid credentials";
                }
            }
            else if (cmd == "AUTH_LOGOUT") {
                g_auth->logout();
                response = "OK";
            }
            else if (cmd == "AUTH_GET_ALL_USERS") {
                auto users = g_auth->getAllUsers();
                response = "OK";
                for (auto& [id, name] : users)
                    response += "|" + to_string(id) + "|" + name;
            }
            else if (cmd == "AUTH_DELETE_USER") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    string deletedUsername = parts[1];
                    int userFolderId = g_fm->getEntityId(deletedUsername, g_fm->getRootId());
                    bool folderOk = true;
                    if (userFolderId > 1)
                        folderOk = g_fm->deleteEntityTree(userFolderId, "admin");
                    bool ok = folderOk && g_auth->deleteUser(deletedUsername);

                    response = ok ? "OK" : "ERR|delete user failed";
                }
            }
            else if (cmd == "AUTH_CREATE_GROUP") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->createGroup(parts[1]);
                    response = ok ? "OK" : "ERR|group creation failed";
                }
            }
            else if (cmd == "AUTH_ADD_USER_TO_GROUP") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->addUserToGroup(parts[1], parts[2]);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "AUTH_REMOVE_USER_FROM_GROUP") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->removeUserFromGroup(parts[1], parts[2]);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "AUTH_GET_ALL_GROUPS") {
                auto groups = g_auth->getAllGroups();
                response = "OK";
                for (auto& g : groups) response += "|" + g;
            }
            else if (cmd == "AUTH_GET_USERS_IN_GROUP") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    auto users = g_auth->getUsersInGroup(parts[1]);
                    response = "OK";
                    for (auto& u : users) response += "|" + u;
                }
            }

            // ── FileManager commands ───────────────────────────────
            else if (cmd == "FM_INIT") {
                bool ok = g_fm->initializeDatabase();
                response = ok ? "OK" : "ERR|init failed";
            }
            else if (cmd == "FM_CREATE_TEXT_FILE") {
                if (parts.size() < 5) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[3]);
                    string username = parts[2];
                    if (!g_fm->checkDirectPermission(parentId, username, "write")) {
                        response = "ERR|permission denied";
                    } else {
                        string content;
                        for (size_t i = 4; i < parts.size(); ++i) {
                            if (i > 4) content += "|";
                            content += parts[i];
                        }
                        bool ok = g_fm->createTextFile(parts[1], username, content, parentId);
                        response = ok ? "OK" : "ERR|create failed";
                    }
                }
            }
            else if (cmd == "FM_CREATE_FOLDER") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[3]);
                    string username = parts[2];
                    if (!g_fm->checkDirectPermission(parentId, username, "write")) {
                        response = "ERR|permission denied";
                    } else {
                        bool ok = g_fm->createFolder(parts[1], username, parentId);
                        response = ok ? "OK" : "ERR|create failed";
                    }
                }
            }
            else if (cmd == "FM_CREATE_BINARY_FILE") {
                if (parts.size() < 5) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[4]);
                    string username = parts[2];
                    if (!g_fm->checkDirectPermission(parentId, username, "write")) {
                        response = "ERR|permission denied";
                    } else {
                        bool ok = g_fm->createBinaryFile(parts[1], username, parts[3], parentId);
                        response = ok ? "OK" : "ERR|create failed";
                    }
                }
            }
            else if (cmd == "FM_RENAME") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->renameEntity(id, parts[2], parts[3]);
                    response = ok ? "OK" : "ERR|rename failed";
                }
            }
            else if (cmd == "FM_DELETE") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->deleteEntity(id, parts[2]);
                    response = ok ? "OK" : "ERR|delete failed";
                }
            }
            else if (cmd == "FM_UPDATE_TEXT_FILE") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    string content;
                    for (size_t i = 3; i < parts.size(); ++i) {
                        if (i > 3) content += "|";
                        content += parts[i];
                    }
                    bool ok = g_fm->updateTextFile(id, content, parts[2]);
                    response = ok ? "OK" : "ERR|update failed";
                }
            }
            else if (cmd == "FM_CHECK_PERMISSION") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->checkPermission(id, parts[2], parts[3]);
                    response = string("OK|") + (ok ? "1" : "0");
                }
            }
            else if (cmd == "FM_CHECK_DIRECT_PERMISSION") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->checkDirectPermission(id, parts[2], parts[3]);
                    response = string("OK|") + (ok ? "1" : "0");
                }
            }
            else if (cmd == "FM_UPDATE_OTHERS_PERM") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool canRead  = parts[2] == "1";
                    bool canWrite = parts[3] == "1";
                    bool ok = g_fm->updateOthersPermissions(id, canRead, canWrite);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_SHARE") {
                if (parts.size() < 5) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool canRead  = parts[3] == "1";
                    bool canWrite = parts[4] == "1";
                    bool ok = g_fm->shareEntity(id, parts[2], canRead, canWrite);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_REVOKE_SHARE") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->revokeShare(id, parts[2]);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_LOAD_PERMISSIONS") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    auto pm = g_fm->loadPermissions(id);
                    auto op = pm.getOthersPermission();
                    auto up = pm.getUserPermission();
                    int oRead  = op ? (op->canRead()  ? 1 : 0) : 0;
                    int oWrite = op ? (op->canWrite() ? 1 : 0) : 0;
                    string members = up ? up->serialize() : "";
                    response = "OK|" + to_string(oRead) + "|" + to_string(oWrite) + "|" + members;
                }
            }
            else if (cmd == "FM_GET_ROOT_ID") {
                response = "OK|" + to_string(g_fm->getRootId());
            }
            else if (cmd == "FM_GET_PARENT_ID") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    int parentId = g_fm->getParentId(id);
                    response = "OK|" + to_string(parentId);
                }
            }
            else if (cmd == "FM_GET_ENTITY_ID") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[2]);
                    int id = g_fm->getEntityId(parts[1], parentId);
                    response = "OK|" + to_string(id);
                }
            }
            else if (cmd == "FM_GET_CHILDREN") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[1]);
                    string username = (parts.size() >= 3) ? parts[2] : "";
                    auto children = g_fm->getChildren(parentId, username);
                    string json = serializeChildrenToJson(children, parentId);
                    response = "OK|" + json;
                }
            }
            else if (cmd == "FM_BUILD_TREE") {
                string username = (parts.size() >= 2) ? parts[1] : "";
                int rootId = (parts.size() >= 3) ? stoi(parts[2]) : g_fm->getRootId();
                auto tree = g_fm->buildTree(rootId, nullptr, username);
                if (!tree) {
                    response = "ERR|failed to build tree";
                } else {
                    string json = serializeTreeToJson(tree.get());
                    response = "OK|" + json;
                }
            }
            else {
                response = "ERR|unknown command: " + cmd;
            }
        } catch (const exception& e) {
            response = string("ERR|") + e.what();
        } catch (...) {
            response = "ERR|unknown exception";
        }

        sendMsg(clientSocket, response);
    }

    closesocket(clientSocket);
    cout << "Client deconectat.\n";
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    vector<string> dbPaths = {
        "C:/Users/marius/Documents/GitHub/virtual-file-system/Program_source/data/filesystem_app.db",
        "Program_source/data/filesystem_app.db",
        "../Program_source/data/filesystem_app.db",
        "../../Program_source/data/filesystem_app.db"
    };
    bool dbOpened = false;
    for (const auto& dbPath : dbPaths) {
        if (!filesystem::exists(dbPath)) {
            continue;
        }

        if (g_db.open(dbPath)) {
            cout << "Baza de date deschisa din: " << filesystem::absolute(dbPath).string() << "\n";
            dbOpened = true;
            break;
        }
    }
    if (!dbOpened) {
        cout << "Eroare: Baza de date nu s-a gasit in nicio locatie cunoscuta!\n";
        cout << "Folder curent: " << filesystem::current_path().string() << "\n";
        return 1;
    }

    g_auth = new AuthService(g_db, g_logger);
    g_fm   = new FileManager(g_db, g_logger);

    g_auth->initializeDatabase();
    g_fm->initializeDatabase();

    // repara inregistrarile OWNER vechi care au can_read=0 / can_write=0 din cauza unui bug al constructorului
    g_db.execute("UPDATE permissions SET can_read=1, can_write=1 WHERE type='OWNER';");

    // root (id=1) e vizibil si writable de oricine (pentru crearea folderului personal)
    g_db.execute("INSERT OR IGNORE INTO permissions(entity_id, type, can_read, can_write) VALUES(1, 'OTHERS', 1, 1);");
    g_db.execute("UPDATE permissions SET can_read=1, can_write=1 WHERE entity_id=1 AND type='OTHERS';"
    );

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    int port = 8080;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, 5);

    cout << "Server ATMosFILE pornit pe portu " + to_string(port) + " ...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;
        thread(handleClient, clientSocket).detach();
    }

    delete g_auth;
    delete g_fm;
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
