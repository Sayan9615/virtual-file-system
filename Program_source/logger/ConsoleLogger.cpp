#include "ConsoleLogger.h"
#include <iostream>

void ConsoleLogger::log(const std::string& event) {
    Logger::log(event);         // salvează în vector
    std::cout << event << "\n"; // printează în consolă
}