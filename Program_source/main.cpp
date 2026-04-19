#include <QApplication>
#include "gui/MainWindow.h"
#include "gui/LoginDialog.h"
#include "services/Database.h"
#include "services/FileManager.h"
#include "services/AuthService.h"
#include "logger/ConsoleLogger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Initializare DB
    Database db;
    db.open("data/filesystem_app.db");

    ConsoleLogger logger;
    AuthService auth(db, logger);
    auth.initializeDatabase();

    FileManager fm(db, logger);
    fm.initializeDatabase();

    // Login
    LoginDialog loginDialog(auth);
    if (loginDialog.exec() != QDialog::Accepted)
        return 0; // userul a inchis fereastra

    QString username = loginDialog.getUsername();

    // Construieste arborele din DB
    auto root = fm.buildTree(fm.getRootId());
    if (!root) {
        fm.createFolder("Documents", username.toStdString(), "users", 1);
        root = fm.buildTree(fm.getRootId());
    }

    MainWindow window(root.get());
    window.setWindowTitle("ATMosFILE - " + username);
    window.show();

    return app.exec();
}