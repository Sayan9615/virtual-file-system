#include <iostream>
#include "logger/TimestampedLogger.h"
#include "services/AuthService.h"
#include "services/Database.h"
#include "services/EventLog.h"
#include "filesystem/TextFile.h"
#include <sodium.h>

int main() {
    if (sodium_init() < 0) {
        std::cout << "Eroare la initializarea libsodium.\n";
        return 1;
    }

    // ── LOGGER ──────────────────────────────────────────────
    TimestampedLogger logger("app.log");
    std::cout << "==============================\n";
    std::cout << "         LOGGER TEST          \n";
    std::cout << "==============================\n";

    // ── AUTH ────────────────────────────────────────────────
    std::cout << "\n==============================\n";
    std::cout << "         AUTH TEST            \n";
    std::cout << "==============================\n";

    Database db;
    if (!db.open("project.db")) {
        std::cout << "Nu s-a putut deschide baza de date.\n";
        return 1;
    }

    AuthService authService(db, logger);
    if (!authService.initializeDatabase()) {
        std::cout << "Nu s-a putut initializa tabela users.\n";
        return 1;
    }

    // Register
    std::cout << "\n-- Register --\n";
    bool reg1 = authService.registerUser("marius", "parola123");
    std::cout << (reg1 ? "OK: marius inregistrat" : "ESUAT: marius exista deja") << "\n";

    bool reg2 = authService.registerUser("marius", "parola123");
    std::cout << (reg2 ? "OK: duplicat inregistrat" : "ESUAT: user duplicat") << "\n";

    bool reg3 = authService.registerUser("ab", "123");
    std::cout << (reg3 ? "OK: date invalide acceptate" : "ESUAT: date invalide respinse corect") << "\n";

    // Login
    std::cout << "\n-- Login --\n";
    bool login1 = authService.login("marius", "parolagresita");
    std::cout << (login1 ? "OK: login" : "ESUAT: parola gresita respinsa corect") << "\n";

    bool login2 = authService.login("inexistent", "parola123");
    std::cout << (login2 ? "OK: login" : "ESUAT: user inexistent respins corect") << "\n";

    bool login3 = authService.login("marius", "parola123");
    std::cout << (login3 ? "OK: login reusit" : "ESUAT: login") << "\n";

    if (authService.isAuthenticated()) {
        std::cout << "User logat: " << authService.getCurrentUser().getUsername() << "\n";
    }

    // Logout
    std::cout << "\n-- Logout --\n";
    authService.logout();
    std::cout << (authService.isAuthenticated() ? "EROARE: inca logat" : "OK: logout reusit") << "\n";

    // ── FILESYSTEM + PERMISSIONS ────────────────────────────
    std::cout << "\n==============================\n";
    std::cout << "     FILESYSTEM TEST          \n";
    std::cout << "==============================\n";

    TextFile file("document.txt", "marius", "admins", "Continut initial");
    file.setLogger(&logger);
    file.display();

    // Read/Write
    std::cout << "\n-- Read/Write --\n";
    std::cout << "Continut: " << file.read() << "\n";
    file.write("Continut nou");
    std::cout << "Dupa write: " << file.read() << "\n";

    // Permissions
    std::cout << "\n-- Permissions --\n";
    std::cout << "marius canRead:  " << (file.getPermissions().canRead("marius")  ? "DA" : "NU") << "\n";
    std::cout << "marius canWrite: " << (file.getPermissions().canWrite("marius") ? "DA" : "NU") << "\n";
    std::cout << "ion canRead:     " << (file.getPermissions().canRead("ion")     ? "DA" : "NU") << "\n";
    std::cout << "ion canWrite:    " << (file.getPermissions().canWrite("ion")    ? "DA" : "NU") << "\n";
    std::cout << "altul canRead:   " << (file.getPermissions().canRead("altul")   ? "DA" : "NU") << "\n";

    // Adauga ion in grup
    std::cout << "\n-- Adauga ion in grupul admins --\n";
    auto group = file.getPermissions().getGroupPermission();
    if (group) {
        group->addMember("ion");
        std::cout << "ion canRead dupa adaugare: "
                  << (file.getPermissions().canRead("ion") ? "DA" : "NU") << "\n";
    }

    // Permission denied
    std::cout << "\n-- Permission Denied --\n";
    if (!file.getPermissions().canWrite("ion")) {
        EventLog(EventType::PERMISSION_DENIED, "ion",
                 "Acces write refuzat la: " + file.getName(), logger);
        std::cout << "ion nu poate scrie in fisier - logat\n";
    }

    // Share
    std::cout << "\n-- Share --\n";
    file.share("maria");
    std::cout << "Shared with: ";
    for (const auto& u : file.getSharedWith())
        std::cout << u << " ";
    std::cout << "\n";

    file.revokeAccess("maria");
    std::cout << "Dupa revocare, shared with: ";
    for (const auto& u : file.getSharedWith())
        std::cout << u << " ";
    std::cout << "(gol)\n";

    // Search
    std::cout << "\n-- Search --\n";
    auto results = file.search("nou");
    std::cout << "Cautare 'nou': " << (results.empty() ? "negasit" : "gasit") << "\n";

    // Rename
    std::cout << "\n-- Rename --\n";
    file.setName("document_nou.txt");
    std::cout << "Nume nou: " << file.getName() << "\n";

    // Serialize
    std::cout << "\n-- Serialize --\n";
    std::cout << "Serializat: " << file.serialize() << "\n";

    std::cout << "\n==============================\n";
    std::cout << "           DONE               \n";
    std::cout << "==============================\n";
    std::cout << "Log salvat in app.log\n";

    return 0;
}