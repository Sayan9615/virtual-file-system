#pragma once
#include <cstdint>

enum class MessageType : uint8_t {
    LOGIN       = 1,
    LOGOUT      = 2,
    RESPONSE    = 3,
    LIST_FILES  = 4,
    UPLOAD      = 5,
    DOWNLOAD    = 6,
    MKDIR       = 7,
    DELETE_FILE = 8,
    REGISTER    = 9,  // ← adaugă asta
};

#pragma pack(push, 1)

struct MessageHeader {
    MessageType type;
    uint32_t    payload_size;
};

struct LoginPayload {
    char username[64];
    char password[64];
};

struct ResponsePayload {
    bool success;
    char message[256];
};

struct FileEntry {
    int     id;
    char    name[256];
    char    ownerUser[64];
    char    ownerGroup[64];
    bool    isFolder;
    int64_t createdAt;
    int64_t modifiedAt;
};

struct FileListHeader {
    uint32_t count;
};



#pragma pack(pop)