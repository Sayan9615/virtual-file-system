#include "SocketClient.h"
#include <cstdio>
#include <cstring>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")

namespace {
constexpr unsigned int MAX_MESSAGE_SIZE = 50u * 1024u * 1024u;

bool isHexHeader(const char* buffer) {
    for (int i = 0; i < 8; ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(buffer[i])))
            return false;
    }
    return true;
}
}

SocketClient::SocketClient() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SocketClient::~SocketClient() {
    disconnect();
    WSACleanup();
}

bool SocketClient::connect(const std::string& host, int port) {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (::connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    return true;
}

void SocketClient::disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool SocketClient::isConnected() const {
    return m_socket != INVALID_SOCKET;
}

bool SocketClient::sendMsg(const std::string& msg) {
    // Send 8-char hex length prefix + payload
    char lenBuf[9];
    std::snprintf(lenBuf, sizeof(lenBuf), "%08X", (unsigned int)msg.size());

    // Send length header
    int headerSent = 0;
    while (headerSent < 8) {
        int sent = send(m_socket, lenBuf + headerSent, 8 - headerSent, 0);
        if (sent <= 0) return false;
        headerSent += sent;
    }

    // Send payload
    size_t total = 0;
    while (total < msg.size()) {
        int n = send(m_socket, msg.c_str() + total, (int)(msg.size() - total), 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

std::string SocketClient::recvMsg() {
    // Read 8-byte length header
    char lenBuf[9] = {0};
    int total = 0;
    while (total < 8) {
        int n = recv(m_socket, lenBuf + total, 8 - total, 0);
        if (n <= 0) return "";
        total += n;
    }

    unsigned int msgLen = 0;
    if (!isHexHeader(lenBuf)) return "";
    std::sscanf(lenBuf, "%08X", &msgLen);

    if (msgLen == 0) return "";
    if (msgLen > MAX_MESSAGE_SIZE) return "";

    std::string result(msgLen, '\0');
    size_t received = 0;
    while (received < msgLen) {
        int n = recv(m_socket, &result[received], (int)(msgLen - received), 0);
        if (n <= 0) return "";
        received += n;
    }
    return result;
}

std::string SocketClient::sendCommand(const std::string& cmd) {
    if (!sendMsg(cmd)) return "ERR|send failed";
    return recvMsg();
}
