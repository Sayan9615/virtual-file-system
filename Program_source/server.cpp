#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstring>
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
#include "filesystem/SharedFolder.h"
#include "filesystem/TextFile.h"
#include "filesystem/BinaryFile.h"

using namespace std;

mutex g_mutex;  // protejeaza accesul la DB din mai multe thread-uri
Database g_db;
ConsoleLogger g_logger;
AuthService* g_auth = nullptr;
FileManager* g_fm   = nullptr;

// ── Protocol helpers ──────────────────────────────────────────────────────────

static bool sendMsg(SOCKET sock, const string& msg) {
    char lenBuf[9];
    snprintf(lenBuf, sizeof(lenBuf), "%08X", (unsigned int)msg.size());

    int sent = send(sock, lenBuf, 8, 0);
    if (sent != 8) return false;

    size_t total = 0;
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

    unsigned int msgLen = 0;
    sscanf(lenBuf, "%08X", &msgLen);
    if (msgLen == 0) return "";

    string result(msgLen, '\0');
    size_t received = 0;
    while (received < msgLen) {
        int n = recv(sock, &result[received], (int)(msgLen - received), 0);
        if (n <= 0) return "";
        received += n;
    }
    return result;
}

static vector<string> splitMsg(const string& s, char delim) {
    vector<string> parts;
    stringstream ss(s);
    string token;
    while (getline(ss, token, delim)) {
        parts.push_back(token);
    }
    return parts;
}

static string jsonEscapeStr(const string& s) {
    string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if (c == '\\') { out += "\\\\"; }
        else if (c == '"')  { out += "\\\""; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\r') { out += "\\r"; }
        else if (c == '\t') { out += "\\t"; }
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
    if (auto* sf = dynamic_cast<SharedFolder*>(entity)) {
        typeStr = "SharedFolder";
    } else if (dynamic_cast<Folder*>(entity)) {
        typeStr = "Folder";
    } else if (dynamic_cast<TextFile*>(entity)) {
        typeStr = "TextFile";
    } else if (dynamic_cast<BinaryFile*>(entity)) {
        typeStr = "BinaryFile";
    } else {
        typeStr = "Unknown";
    }

    json += "\"type\":\"" + typeStr + "\",";
    json += "\"name\":\"" + jsonEscapeStr(entity->getName()) + "\",";
    json += "\"owner\":\"" + jsonEscapeStr(entity->getOwnerUser()) + "\",";
    json += "\"group\":\"" + jsonEscapeStr(entity->getOwnerGroup()) + "\",";
    json += "\"parentId\":" + to_string(parentId) + ",";
    json += "\"createdAt\":" + to_string((long long)entity->getCreatedAt()) + ",";
    json += "\"modifiedAt\":" + to_string((long long)entity->getModifiedAt());

    if (auto* tf = dynamic_cast<TextFile*>(entity)) {
        json += ",\"content\":\"" + jsonEscapeStr(tf->read()) + "\"";
    } else if (auto* bf = dynamic_cast<BinaryFile*>(entity)) {
        json += ",\"extension\":\"" + jsonEscapeStr(bf->getExtension()) + "\",\"size\":" + to_string(bf->getSize());
    } else if (auto* sf = dynamic_cast<SharedFolder*>(entity)) {
        json += ",\"isPublic\":" + string(sf->isPublic() ? "1" : "0");
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
            if (childFolder) {
                serializeTreeDFS(childFolder, folder->getId(), out, first);
            }
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

// ── Handle client ─────────────────────────────────────────────────────────────

void handleClient(SOCKET clientSocket) {
    cout << "Client nou conectat.\n";

    while (true) {
        string msg = recvMsg(clientSocket);
        if (msg.empty()) break;

        cout << "Primit: " << msg.substr(0, 120) << "\n";

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
                for (auto& [id, name] : users) {
                    response += "|" + to_string(id) + "|" + name;
                }
            }
            // 🔥 Adaugat: AUTH_DELETE_USER
            else if (cmd == "AUTH_DELETE_USER") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    bool ok = g_auth->deleteUser(parts[1]);
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
                // FM_CREATE_TEXT_FILE|name|ownerUser|ownerGroup|parentId|content
                if (parts.size() < 6) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[4]);
                    string content;
                    for (size_t i = 5; i < parts.size(); ++i) {
                        if (i > 5) content += "|";
                        content += parts[i];
                    }
                    bool ok = g_fm->createTextFile(parts[1], parts[2], parts[3], content, parentId);
                    response = ok ? "OK" : "ERR|create failed";
                }
            }
            else if (cmd == "FM_CREATE_FOLDER") {
                if (parts.size() < 5) { response = "ERR|missing args"; }
                else {
                    int parentId = stoi(parts[4]);
                    bool ok = g_fm->createFolder(parts[1], parts[2], parts[3], parentId);
                    response = ok ? "OK" : "ERR|create failed";
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
                    std::string content;
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
            else if (cmd == "FM_SET_PERMISSION") {
                if (parts.size() < 5) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool canRead  = parts[3] == "1";
                    bool canWrite = parts[4] == "1";
                    bool ok = g_fm->setPermission(id, parts[2], canRead, canWrite);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_UPDATE_GROUP_PERM") {
                if (parts.size() < 4) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool canRead  = parts[2] == "1";
                    bool canWrite = parts[3] == "1";
                    bool ok = g_fm->updateGroupPermissions(id, canRead, canWrite);
                    response = ok ? "OK" : "ERR|failed";
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
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->shareEntity(id, parts[2]);
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
            else if (cmd == "FM_GET_SHARED_WITH") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    auto users = g_fm->getSharedWithList(id);
                    response = "OK";
                    for (auto& u : users) response += "|" + u;
                }
            }
            else if (cmd == "FM_ADD_USER_TO_GROUP") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->addUserToGroup(id, parts[2]);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_REMOVE_USER_FROM_GROUP") {
                if (parts.size() < 3) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    bool ok = g_fm->removeUserFromGroup(id, parts[2]);
                    response = ok ? "OK" : "ERR|failed";
                }
            }
            else if (cmd == "FM_LOAD_PERMISSIONS") {
                if (parts.size() < 2) { response = "ERR|missing args"; }
                else {
                    int id = stoi(parts[1]);
                    auto pm = g_fm->loadPermissions(id);
                    auto gp = pm.getGroupPermission();
                    auto op = pm.getOthersPermission();
                    int gRead  = gp ? (gp->canRead()  ? 1 : 0) : 0;
                    int gWrite = gp ? (gp->canWrite() ? 1 : 0) : 0;
                    int oRead  = op ? (op->canRead()  ? 1 : 0) : 0;
                    int oWrite = op ? (op->canWrite() ? 1 : 0) : 0;
                    response = "OK|" + to_string(gRead) + "|" + to_string(gWrite) +
                               "|" + to_string(oRead) + "|" + to_string(oWrite);
                }
            }
            else if (cmd == "FM_GET_ROOT_ID") {
                int rootId = g_fm->getRootId();
                response = "OK|" + to_string(rootId);
            }
            else if (cmd == "FM_BUILD_TREE") {
                string username = (parts.size() >= 2) ? parts[1] : "";
                int rootId = g_fm->getRootId();
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
        "../Program_source/data/filesystem_app.db"
    };
    bool dbOpened = false;
    for (int i = 0; i < dbPaths.size();i++) {
        if (g_db.open(dbPaths[i])) { dbOpened = true; break; }
    }
    if (!dbOpened) {
        cout << "Eroare: Baza de date nu s-a gasit in nicio locatie cunoscuta!\n";
        return 1;
    }

    g_auth = new AuthService(g_db, g_logger);
    g_fm   = new FileManager(g_db, g_logger);

    g_auth->initializeDatabase();
    g_fm->initializeDatabase();

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Allow address reuse
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    int port = 8080;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, 5);

    cout << "Server ATMosFILE pornit pe portul " + to_string(port) + " ...\n";

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