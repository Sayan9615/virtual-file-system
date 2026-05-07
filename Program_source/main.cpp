#include <QApplication>
#include <QMessageBox>
#include <iostream>

#include "services/Database.h"
#include "services/FileManager.h"
#include "services/AuthService.h"
#include "logger/ConsoleLogger.h"
#include "external/PasswordHasher.h"
#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {

    // ── Qt Application ───────────────────────────────
    QApplication app(argc, argv);
    app.setApplicationName("ATMosFS");
    app.setApplicationVersion("1.0");

    // ── Initializare libsodium ───────────────────────
    if (!PasswordHasher::initialize()) {
        QMessageBox::critical(nullptr,
                              "Eroare",
                              "Eroare la initializare libsodium!");
        return 1;
    }

    // ── Initializare baza de date ────────────────────
    Database db;
    if (!db.open("data/filesystem_app.db")) {
        QMessageBox::critical(nullptr,
                              "Eroare",
                              "Nu pot deschide baza de date!");
        return 1;
    }

    // ── Initializare servicii ────────────────────────
    ConsoleLogger logger;
    AuthService   auth(db, logger);
    FileManager   fm(db, logger);

    if (!auth.initializeDatabase()) {
        QMessageBox::critical(nullptr,
                              "Eroare",
                              "Eroare la initializare auth DB!");
        return 1;
    }

    if (!fm.initializeDatabase()) {
        QMessageBox::critical(nullptr,
                              "Eroare",
                              "Eroare la initializare FileManager DB!");
        return 1;
    }

    // ── Login Dialog ─────────────────────────────────
    LoginDialog loginDialog(auth);
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0; // userul a inchis fereastra
    }

    std::string username = loginDialog.getUsername().toStdString();

    // ── Construieste arborele din DB ─────────────────
    auto root = fm.buildTree(fm.getRootId(), nullptr, username);
    if (!root) {
        QMessageBox::critical(nullptr,
                              "Eroare",
                              "Eroare la incarcarea arborelui!");
        return 1;
    }

    // ── Date de test daca DB e goala ─────────────────
    if (root->getChildCount() == 0) {
        fm.createFolder("Documents", username, "users", 1);
        fm.createFolder("Images",    username, "users", 1);

        int docsId = fm.getEntityId("Documents", 1);
        int imgsId = fm.getEntityId("Images",    1);

        fm.createTextFile("readme",   username, "users",
                          "Hello ATMosFS! Acesta e un fisier de test.", docsId);
        fm.createTextFile("notes",    username, "users",
                          "Note importante despre proiect.", docsId);
        fm.createBinaryFile("photo",  username, "users", ".jpg", imgsId);
        fm.createBinaryFile("backup", username, "users", ".bin", docsId);

        root = fm.buildTree(fm.getRootId(), nullptr, username);
    }

    // ── Main Window ──────────────────────────────────
    MainWindow mainWindow(root.get(),username);
    mainWindow.show();

    return app.exec();
}