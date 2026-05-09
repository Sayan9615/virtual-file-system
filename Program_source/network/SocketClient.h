#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

class SocketClient {
public:
    SocketClient();
    ~SocketClient();
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;
    std::string sendCommand(const std::string& cmd);
private:
    SOCKET m_socket = INVALID_SOCKET;
    bool sendMsg(const std::string& msg);
    std::string recvMsg();
};
