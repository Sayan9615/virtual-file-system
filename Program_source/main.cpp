#include "logger/ConsoleLogger.h"
#include "services/Database.h"
#include <iostream>

int main(){
    Database db;
    db.open("../Program_source/data/filesystem_app.db");
    if(db.getConnection() == NULL){
        std::cout <<"Eroare deschidere baza de date";
    }


    return 0;
}