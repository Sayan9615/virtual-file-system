#include <iostream>
#include "logger/TimestampedLogger.h"
#include "services/AuthService.h"
#include "services/Database.h"
#include "services/EventLog.h"
#include "filesystem/TextFile.h"
#include "services/FileManager.h"
#include <sodium.h>

int main() {
    if (sodium_init() < 0) {
        std::cout << "Eroare la initializarea libsodium.\n";
        return 1;
    }


TimestampedLogger logger("app.log");
Database db;
db.open("project.db");

FileManager fm(db, logger);
fm.initializeDatabase();

// Creare
int mariusId = fm.getEntityId("marius", 1);
// Listare
auto children = fm.getChildren(mariusId);
for (auto& c : children)
    c->display();

std::cout << "\n==============================\n";
std::cout << "           DONE               \n";
std::cout << "==============================\n";

return 0;
}