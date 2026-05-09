#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (sockaddr*)&addr, sizeof(addr));
    cout << "Conectat la server!\n";

    while (true) {
        cout << "\n1. Login\n2. Register\n0. Iesire\nOptiunea: ";
        int optiune;
        cin >> optiune;

        if (optiune == 0) break;

        string username, parola;
        cout << "Username: "; cin >> username;
        cout << "Parola: ";   cin >> parola;

        string comanda = (optiune == 1 ? "login" : "register");
        string mesaj   = comanda + ":" + username + ":" + parola;

        send(sock, mesaj.c_str(), mesaj.size(), 0);

        // Primeste raspuns
        char buffer[1024] = {0};
        recv(sock, buffer, sizeof(buffer), 0);

        string raspuns(buffer);
        if (raspuns.substr(0, 2) == "ok")
            cout << "[OK] "     << raspuns.substr(3) << "\n";
        else
            cout << "[EROARE] " << raspuns.substr(7) << "\n";
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}