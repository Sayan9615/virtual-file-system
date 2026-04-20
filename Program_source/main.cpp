#include "services/Database.h"
#include "services/FileManager.h"
#include "services/AuthService.h"
#include "logger/ConsoleLogger.h"
#include "filesystem/TextFile.h"
#include "filesystem/Folder.h"
#include "services/SearchEngine.h"
#include "external/PasswordHasher.h"
#include <iostream>
#include <string>
#include <memory>

void printTree(Folder* folder, int depth = 0) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << "[" << folder->getId() << "] "
              << folder->getIcon() << " " << folder->getName()
              << " (" << folder->getChildCount() << " elemente)\n";

    for (const auto& child : folder->getChildren()) {
        if (child->isFolder()) {
            printTree(static_cast<Folder*>(child.get()), depth + 1);
        } else {
            std::string childIndent((depth + 1) * 2, ' ');
            std::cout << childIndent << "[" << child->getId() << "] "
                      << child->getIcon() << " " << child->getName()
                      << " (" << child->getSize() << " B)\n";
        }
    }
}

void printMenu(const std::string& username) {
    std::cout << "\n===== ATMosFILE - " << username << " =====\n";
    std::cout << "1. Afiseaza arborele\n";
    std::cout << "2. Creeaza folder\n";
    std::cout << "3. Creeaza fisier text\n";
    std::cout << "4. Citeste fisier text\n";
    std::cout << "5. Sterge entitate\n";
    std::cout << "6. Cauta (nume sau continut)\n";
    std::cout << "7. Test SearchEngine\n";
    std::cout << "8. Test permisiuni grup\n";
    std::cout << "9. Afiseaza toti utilizatorii\n";
    std::cout << "0. Logout & Iesire\n";
    std::cout << "Optiune: ";
}

int main() {
    // ── Initializare ──────────────────────────────────────────────
    if (!PasswordHasher::initialize()) {
        std::cerr << "Eroare la initializare libsodium!\n";
        return 1;
    }

    Database db;
    if (!db.open("Program_source/data/filesystem_app.db")) {
        std::cerr << "Nu pot deschide baza de date!\n";
        return 1;
    }

    ConsoleLogger logger;
    AuthService auth(db, logger);
    FileManager fm(db, logger);
    SearchEngine searchEngine;

    if (!auth.initializeDatabase()) {
        std::cerr << "Eroare la initializare auth DB!\n";
        return 1;
    }
    if (!fm.initializeDatabase()) {
        std::cerr << "Eroare la initializare FM DB!\n";
        return 1;
    }

    // ── Login / Register ──────────────────────────────────────────
    std::string username, password;
    std::cout << "===== ATMosFILE =====\n";
    std::cout << "1. Login\n2. Inregistrare\nOptiune: ";
    int authChoice; std::cin >> authChoice; std::cin.ignore();

    std::cout << "Username: "; std::getline(std::cin, username);
    std::cout << "Parola: ";   std::getline(std::cin, password);

    if (authChoice == 2) {
        if (auth.registerUser(username, password)) {
            std::cout << "Cont creat cu succes!\n";
        } else {
            std::cout << "Eroare la inregistrare!\n";
            return 1;
        }
    }

    if (!auth.login(username, password)) {
        std::cout << "Username sau parola incorecta!\n";
        return 1;
    }

    std::cout << "Bun venit, " << username << "!\n";

    // ── Construieste arborele ─────────────────────────────────────
    auto root = fm.buildTree(fm.getRootId(), nullptr, username);
    if (!root) {
        std::cerr << "Eroare la incarcarea arborelui!\n";
        return 1;
    }

    // Date de test daca arborele e gol
    if (root->getChildCount() == 0) {
        std::cout << "DB goala - adaug date de test...\n";
        fm.createFolder("Documents", username, "users", 1);
        fm.createFolder("Images",    username, "users", 1);

        int docsId = fm.getEntityId("Documents", 1);
        int imgsId = fm.getEntityId("Images", 1);

        fm.createTextFile("readme",   username, "users", "Hello VFS! Acesta e un fisier de test.", docsId);
        fm.createTextFile("notes",    username, "users", "Note importante despre proiect.", docsId);
        fm.createTextFile("todo",     username, "users", "1. Implementeaza search\n2. Fa GUI\n3. Profit", docsId);
        fm.createBinaryFile("photo",  username, "users", ".jpg", imgsId);
        fm.createBinaryFile("backup", username, "users", ".bin", docsId);

        root = fm.buildTree(fm.getRootId(), nullptr, username);
        std::cout << "Date de test adaugate!\n";
    }

    // ── Meniu principal ───────────────────────────────────────────
    int choice = -1;
    while (choice != 0) {
        printMenu(username);
        std::cin >> choice; std::cin.ignore();

        switch (choice) {
            case 1: {
                std::cout << "\n--- Arbore VFS ---\n";
                printTree(root.get());
                break;
            }

            case 2: {
                std::string name, group;
                int parentId;
                std::cout << "Nume folder: ";          std::getline(std::cin, name);
                std::cout << "Grup: ";                 std::getline(std::cin, group);
                std::cout << "Parent ID (1 = root): "; std::cin >> parentId; std::cin.ignore();

                if (fm.createFolder(name, username, group, parentId)) {
                    std::cout << "Folder creat!\n";
                    root = fm.buildTree(fm.getRootId(), nullptr, username);
                } else {
                    std::cout << "Eroare la creare folder!\n";
                }
                break;
            }

            case 3: {
                std::string name, content, group;
                int parentId;
                std::cout << "Nume fisier (fara extensie): "; std::getline(std::cin, name);
                std::cout << "Grup: ";                        std::getline(std::cin, group);
                std::cout << "Parent ID (1 = root): ";        std::cin >> parentId; std::cin.ignore();
                std::cout << "Continut: ";                    std::getline(std::cin, content);

                if (fm.createTextFile(name, username, group, content, parentId)) {
                    std::cout << "Fisier creat!\n";
                    root = fm.buildTree(fm.getRootId(), nullptr, username);
                } else {
                    std::cout << "Eroare la creare fisier!\n";
                }
                break;
            }

            case 4: {
                int id;
                std::cout << "ID fisier: "; std::cin >> id; std::cin.ignore();
                if (!fm.checkPermission(id, username, "read")) {
                    std::cout << "Acces refuzat!\n";
                    break;
                }
                auto file = fm.getTextFile(id);
                if (file) {
                    std::cout << "\n--- Continut ---\n" << file->read() << "\n";
                } else {
                    std::cout << "Fisier negasit!\n";
                }
                break;
            }

            case 5: {
                int id;
                std::cout << "ID entitate de sters: "; std::cin >> id; std::cin.ignore();
                if (fm.deleteEntity(id, username)) {
                    std::cout << "Sters!\n";
                    root = fm.buildTree(fm.getRootId(), nullptr, username);
                } else {
                    std::cout << "Eroare sau permisiune refuzata!\n";
                }
                break;
            }

            case 6: {
                std::string query;
                std::cout << "Termen de cautare: "; std::getline(std::cin, query);
                auto results = searchEngine.search(root.get(), query);
                if (results.empty()) {
                    std::cout << "Niciun rezultat.\n";
                } else {
                    std::cout << "\n--- Rezultate (" << results.size() << ") ---\n";
                    for (const auto& r : results) {
                        std::cout << "  [" << r.entity->getId() << "] " << r.absolutePath << "\n";
                        for (const auto& line : r.matchingLines)
                            std::cout << "    -> " << line << "\n";
                    }
                }
                break;
            }

            case 7: {
                std::cout << "\n--- Test SearchEngine ---\n";
                std::cout << "1. Cauta dupa nume (wildcard)\n";
                std::cout << "2. Cauta dupa continut\n";
                std::cout << "3. Cauta dupa extensie\n";
                std::cout << "4. Cauta dupa owner\n";
                std::cout << "Optiune: ";
                int searchChoice; std::cin >> searchChoice; std::cin.ignore();

                std::string query;
                std::vector<SearchEngine::SearchResult> results;

                switch (searchChoice) {
                    case 1:
                        std::cout << "Pattern (ex: *.txt, doc*): ";
                        std::getline(std::cin, query);
                        results = searchEngine.searchByName(root.get(), query);
                        break;
                    case 2:
                        std::cout << "Continut de cautat: ";
                        std::getline(std::cin, query);
                        results = searchEngine.searchByContent(root.get(), query);
                        break;
                    case 3:
                        std::cout << "Extensie (ex: txt, .bin): ";
                        std::getline(std::cin, query);
                        results = searchEngine.searchByExtension(root.get(), query);
                        break;
                    case 4:
                        std::cout << "Owner: ";
                        std::getline(std::cin, query);
                        results = searchEngine.searchByOwner(root.get(), query);
                        break;
                }

                if (results.empty()) {
                    std::cout << "Niciun rezultat.\n";
                } else {
                    std::cout << "\n--- " << results.size() << " rezultate ---\n";
                    for (const auto& r : results) {
                        std::cout << "  [" << r.entity->getId() << "] " << r.absolutePath << "\n";
                        for (const auto& line : r.matchingLines)
                            std::cout << "    -> " << line << "\n";
                    }
                }
                break;
            }

            case 8: {
                std::cout << "\n--- Test Permisiuni Grup ---\n";
                std::cout << "1. Afiseaza grupurile unei entitati\n";
                std::cout << "2. Adauga user la grupul unei entitati\n";
                std::cout << "3. Verifica acces user la entitate\n";
                std::cout << "Optiune: ";
                int permChoice; std::cin >> permChoice; std::cin.ignore();

                int entityId;
                std::cout << "ID entitate: "; std::cin >> entityId; std::cin.ignore();

                switch (permChoice) {
                    case 1: {
                        auto groups = fm.getEntityGroups(entityId);
                        if (groups.empty()) {
                            std::cout << "Nicio permisiune de grup gasita.\n";
                        } else {
                            for (const auto& g : groups) {
                                std::cout << "  Grup: " << g.getName() << "\n";
                                if (g.getMembers().empty()) {
                                    std::cout << "    (niciun membru)\n";
                                } else {
                                    for (const auto& m : g.getMembers())
                                        std::cout << "    - " << m << "\n";
                                }
                            }
                        }
                        break;
                    }
                    case 2: {
                        std::string user;
                        std::cout << "Username de adaugat: "; std::getline(std::cin, user);
                        if (fm.addUserToGroup(entityId, user))
                            std::cout << "User '" << user << "' adaugat in grup!\n";
                        else
                            std::cout << "Eroare: entitate negasita sau fara grup.\n";
                        break;
                    }
                    case 3: {
                        std::string user, op;
                        std::cout << "Username: "; std::getline(std::cin, user);
                        std::cout << "Operatie (read/write): "; std::getline(std::cin, op);
                        bool ok = fm.checkPermission(entityId, user, op);
                        std::cout << "Acces " << op << ": " << (ok ? "PERMIS" : "REFUZAT") << "\n";
                        break;
                    }
                    default:
                        std::cout << "Optiune invalida!\n";
                }
                break;
            }

            case 9: {
                auto users = auth.getAllUsers();
                std::cout << "\n--- Utilizatori (" << users.size() << ") ---\n";
                for (const auto& [id, name] : users)
                    std::cout << "  [" << id << "] " << name
                              << (name == username ? " (tu)" : "") << "\n";
                break;
            }

            case 0:
                auth.logout();
                std::cout << "La revedere!\n";
                break;

            default:
                std::cout << "Optiune invalida!\n";
        }
    }

    return 0;
}