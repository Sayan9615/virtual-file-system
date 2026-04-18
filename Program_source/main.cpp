#include <iostream>
#include "filesystem/BPlusTree.h"
#include "Database.h"
using namespace std;

int main() {

    BPlusTree<string, int> tree(3);

    // ── INSERT ──────────────────────────────────────────────
    cout << "==============================" << endl;
    cout << "         INSERT               " << endl;
    cout << "==============================" << endl;

    tree.insert("/root/ion/fisier1.txt", 1);
    tree.insert("/root/ion/fisier2.txt", 2);
    tree.insert("/root/ion/fisier3.txt", 3);
    tree.insert("/root/maria/doc.txt",   4);
    tree.insert("/root/maria/img.png",   5);
    tree.insert("/root/admin/config.txt",6);
    tree.insert("/root/ion/fisier4.txt", 7);
    tree.insert("/root/ion/fisier5.txt", 8);

    cout << "Arbore dupa inserare:" << endl;
    tree.display();

    // ── SEARCH ──────────────────────────────────────────────
    cout << "\n==============================" << endl;
    cout << "         SEARCH               " << endl;
    cout << "==============================" << endl;

    // cheie existenta
    auto r1 = tree.search("/root/ion/fisier3.txt");
    cout << "Caut /root/ion/fisier3.txt -> ";
    if (r1.has_value())
        cout << "GASIT, valoare: " << r1.value() << endl;
    else
        cout << "NU a fost gasit!" << endl;

    // cheie inexistenta
    auto r2 = tree.search("/root/inexistent.txt");
    cout << "Caut /root/inexistent.txt  -> ";
    if (r2.has_value())
        cout << "GASIT, valoare: " << r2.value() << endl;
    else
        cout << "NU a fost gasit!" << endl;

    // prima cheie
    auto r3 = tree.search("/root/ion/fisier1.txt");
    cout << "Caut /root/ion/fisier1.txt -> ";
    if (r3.has_value())
        cout << "GASIT, valoare: " << r3.value() << endl;
    else
        cout << "NU a fost gasit!" << endl;

    // ultima cheie
    auto r4 = tree.search("/root/maria/img.png");
    cout << "Caut /root/maria/img.png   -> ";
    if (r4.has_value())
        cout << "GASIT, valoare: " << r4.value() << endl;
    else
        cout << "NU a fost gasit!" << endl;

    // ── RANGE SEARCH ────────────────────────────────────────
    cout << "\n==============================" << endl;
    cout << "       RANGE SEARCH           " << endl;
    cout << "==============================" << endl;

    cout << "Fisierele lui ion:" << endl;
    auto results1 = tree.rangeSearch(
        "/root/ion/fisier1.txt",
        "/root/ion/fisier5.txt"
    );
    if (results1.empty())
        cout << "  Niciun rezultat!" << endl;
    for (auto& v : results1)
        cout << "  -> " << v << endl;

    cout << "Fisierele lui maria:" << endl;
    auto results2 = tree.rangeSearch(
        "/root/maria/doc.txt",
        "/root/maria/img.png"
    );
    if (results2.empty())
        cout << "  Niciun rezultat!" << endl;
    for (auto& v : results2)
        cout << "  -> " << v << endl;

    // interval fara rezultate
    cout << "Interval inexistent:" << endl;
    auto results3 = tree.rangeSearch(
        "/root/zzz/a.txt",
        "/root/zzz/z.txt"
    );
    if (results3.empty())
        cout << "  Niciun rezultat!" << endl;
    for (auto& v : results3)
        cout << "  -> " << v << endl;

    // ── UPDATE ──────────────────────────────────────────────
    cout << "\n==============================" << endl;
    cout << "         UPDATE               " << endl;
    cout << "==============================" << endl;

    cout << "Valoare initiala fisier1: " << tree.search("/root/ion/fisier1.txt").value() << endl;
    tree.insert("/root/ion/fisier1.txt", 99);
    cout << "Valoare dupa update:      " << tree.search("/root/ion/fisier1.txt").value() << endl;

    // ── REMOVE ──────────────────────────────────────────────
    cout << "\n==============================" << endl;
    cout << "         REMOVE               " << endl;
    cout << "==============================" << endl;

    // stergere simpla
    cout << "Sterg /root/ion/fisier3.txt ..." << endl;
    tree.remove("/root/ion/fisier3.txt");

    auto deleted1 = tree.search("/root/ion/fisier3.txt");
    cout << "Caut dupa stergere -> ";
    if (!deleted1.has_value())
        cout << "Confirmat: nu mai exista!" << endl;
    else
        cout << "EROARE: inca exista!" << endl;

    cout << "\nArbore dupa prima stergere:" << endl;
    tree.display();

    // stergeri multiple
    cout << "\nSterg /root/maria/doc.txt ..." << endl;
    tree.remove("/root/maria/doc.txt");

    cout << "Sterg /root/admin/config.txt ..." << endl;
    tree.remove("/root/admin/config.txt");

    cout << "Sterg /root/ion/fisier5.txt ..." << endl;
    tree.remove("/root/ion/fisier5.txt");

    cout << "\nArbore dupa stergerile multiple:" << endl;
    tree.display();

    // stergere cheie inexistenta
    cout << "\nSterg cheie inexistenta /root/ghost.txt ..." << endl;
    tree.remove("/root/ghost.txt");

    // stergere tot ce a ramas
    cout << "\nSterg toate cheile ramase..." << endl;
    tree.remove("/root/ion/fisier1.txt");
    tree.remove("/root/ion/fisier2.txt");
    tree.remove("/root/ion/fisier4.txt");
    tree.remove("/root/maria/img.png");

    cout << "\nArbore dupa stergerea tuturor cheilor:" << endl;
    tree.display();

    // ── FINAL ───────────────────────────────────────────────
    cout << "\n==============================" << endl;
    cout << "           DONE               " << endl;
    cout << "==============================" << endl;

    //TESTARE DATABASE START
    cout << "\n==============================" << endl;
    cout << "         START DB TEST         " << endl;
    cout << "==============================" << endl;
    Database database;

    if (!database.open("../Program_source/data/filesystem_app.db")) {
        std::cout << "Nu s-a putut deschide baza de date.\n";
        return 1;
    }

    std::string createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL
        );
    )";

    if (!database.execute(createUsersTable)) {
        std::cout << "Nu s-a putut crea tabela users.\n";
        return 1;
    }

    std::cout << "Baza de date si tabela users au fost create cu succes.\n";

    database.close();

    cout << "\n==============================" << endl;
    cout << "         END DB TEST         " << endl;
    cout << "==============================" << endl;

    //TESTARE DATA BASE END

    return 0;
}