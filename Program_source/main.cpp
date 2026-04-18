#include <iostream>
#include <memory>
#include "filesystem/BPlusTree.h"
#include "filesystem/Folder.h"
#include "filesystem/SharedFolder.h"
#include "filesystem/TextFile.h"
#include "filesystem/BinaryFile.h"

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
    return 0;
}