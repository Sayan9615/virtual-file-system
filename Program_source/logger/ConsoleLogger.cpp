#include "ConsoleLogger.h"
#include <iostream>

void ConsoleLogger::log(const std::string& event) {
    Logger::log(event);         // salvează în vector
    std::cout << event << "\n"; // printează în consolă
}

void ConsoleLogger::log_timed(const std::string& event){
    Logger::log_timed(event);
    std::cout << Logger::get_time() <<" "<< event<<"\n";
}