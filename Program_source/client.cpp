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

    auto rootFolder = fm.buildTree(fm.getRootId(), nullptr, username);
    if (!rootFolder) {
        QMessageBox::critical(nullptr, "Eroare", "Nu s-a putut incarca sistemul de fisiere!");
        return 1;
    }

    MainWindow window(rootFolder.get(), username, fm, auth);
    window.show();

    return app.exec();
}
