#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "services/AuthService.h"
#include "services/Database.h"
#include "logger/ConsoleLogger.h"

using namespace std;

Database g_db;
ConsoleLogger g_logger;

void handleClient(SOCKET clientSocket) {
    AuthService auth(g_db, g_logger);
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (n <= 0) break;

        string mesaj(buffer);
        cout << "Primit: " << mesaj << "\n";

        // Sparge "comanda:username:parola"
        stringstream ss(mesaj);
        string comanda, username, parola;
        getline(ss, comanda,  ':');
        getline(ss, username, ':');
        getline(ss, parola,   ':');

        string raspuns;

        if (comanda == "login") {
    try {
        bool ok = auth.login(username, parola);
        raspuns = "ok:Login reusit!";
    } catch (const std::exception& e) {
        raspuns = std::string("eroare:") + e.what();
    }
}
else if (comanda == "register") {
    try {
        bool ok = auth.registerUser(username, parola);
        raspuns = "ok:Cont creat cu succes!";
    } catch (const std::exception& e) {
        raspuns = std::string("eroare:") + e.what();
    }
}

        send(clientSocket, raspuns.c_str(), raspuns.size(), 0);
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    
    if(!g_db.open("../Program_source/data/filesystem_app.db")){

        cout << "Baza de date nu s-a deschis";
        return 1;
    }
    
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket,   (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, 5);

    cout << "Server pornit pe portul 8080...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        cout << "Client nou conectat.\n";
        handleClient(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}   