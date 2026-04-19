#include <iostream>
#include <memory>
#include "filesystem/BPlusTree.h"
#include "filesystem/Folder.h"
#include "filesystem/SharedFolder.h"
#include "filesystem/TextFile.h"
#include "filesystem/BinaryFile.h"
#include "logger/TimestampedLogger.h"
#include "logger/ConsoleLogger.h"

#include <sodium.h>
#include "logger/TimestampedLogger.h"
#include "services/AuthService.h"

using namespace std;

int main() {

   try{
    auto root = std::make_shared<Folder>("root", "admin", "admin_group");
    
    auto txt1 = std::make_shared<TextFile>("parole", "admin", "admin_group", "Parola de la server este admin123.");
    auto bin1 = std::make_shared<BinaryFile>("poza_profil", "admin", "admin_group", "date_binare_aici");

    auto publicFolder = std::make_shared<SharedFolder>("Public_Docs", "admin", "admin_group", root.get(), true);
    auto txt2 = std::make_shared<TextFile>("anunt", "guest", "users", "Salutari tuturor!");

    root->addChild(txt1);
    root->addChild(bin1);
    root->addChild(publicFolder);
    publicFolder->addChild(txt2);

    std::cout << "Sistem creat cu succes!\n\n";

    root->display();
    std::cout << "\n";

    auto results = root->search("admin123");

    if (results.empty()) 
    {
        std::cout << "Nu am gasit nimic.\n";
    } 
    else 
    {
        for (const auto& res : results) 
        {
            std::cout << " [!] Gasit la calea: " << res << "\n";
        }
    }
        std::cout << "\n";

    std::cout << "String generat pentru fisierul txt1:\n";
    std::cout << txt1->serialize() << "\n";    

   }
   catch(const std::exception& e)
   {
        std::cerr<<"\n[EROARE_FATALA]: "<<e.what()<<"\n";
   }

    TimestampedLogger fileLog("app.log");
    ConsoleLogger consoleLog;

    fileLog.log("Aplicatie pornita");
    consoleLog.log("Aplicatie pornita");

    if (sodium_init() < 0) return 1;

    // Initializare logger
    TimestampedLogger logger("app.log");

    // Initializare DB si AuthService
    Database db;
    if (!db.open("project.db")) return 1;

    AuthService authService(db, logger);
    if (!authService.initializeDatabase()) return 1;

    // Test register
    std::cout << "=== REGISTER ===\n";
    bool reg = authService.registerUser("marius", "parola123");
    std::cout << (reg ? "Register OK" : "Register ESUAT") << "\n";

    // Test register duplicat
    bool reg2 = authService.registerUser("marius", "parola123");
    std::cout << (reg2 ? "Register OK" : "Register ESUAT - user exista deja") << "\n";

    // Test login gresit
    std::cout << "\n=== LOGIN ===\n";
    bool loginGresit = authService.login("marius", "parolagresita");
    std::cout << (loginGresit ? "Login OK" : "Login ESUAT - parola gresita") << "\n";

    // Test login corect
    bool loginCorect = authService.login("marius", "parola123");
    std::cout << (loginCorect ? "Login OK" : "Login ESUAT") << "\n";

    // Test user curent
    if (authService.isAuthenticated()) {
        std::cout << "User logat: " << authService.getCurrentUser().getUsername() << "\n";
    }

    // Test logout
    std::cout << "\n=== LOGOUT ===\n";
    authService.logout();
    std::cout << (authService.isAuthenticated() ? "Inca logat" : "Logout OK") << "\n";



    return 0;
}