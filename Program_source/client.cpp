#include <QApplication>
#include <QMessageBox>
#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"
#include "network/SocketClient.h"
#include "network/RemoteAuthService.h"
#include "network/RemoteFileManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ATMosFS");

    SocketClient client;
    if (!client.connect("127.0.0.1", 8080)) {
        QMessageBox::critical(nullptr, "Eroare conexiune",
            "Nu s-a putut conecta la server!\nAsigurati-va ca serverul (ATMosFILE_Server) ruleaza.");
        return 1;
    }

    RemoteAuthService auth(client);
    RemoteFileManager fm(client);

    LoginDialog loginDlg(auth);
    if (loginDlg.exec() != QDialog::Accepted) return 0;

    std::string username = loginDlg.getUsername().toStdString();

    try {
    int userFolderId = fm.getEntityId(username, fm.getRootId());
    if (userFolderId == -1) {
        fm.createFolder(username, username, fm.getRootId());
    }
    } catch (...) {
        // folderul exista deja, ignoram
    }

    std::cout << "Inainte de MainWindow\n";
    MainWindow window(username, fm, auth);
    std::cout << "Dupa MainWindow\n";
    window.show();

    return app.exec();
}