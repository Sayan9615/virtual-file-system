#include "FileLogger.h"
#include <stdexcept>

FileLogger::FileLogger(const std::string& filename) {
    logFile.open(filename, std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("Nu s-a putut deschide fisierul: " + filename);
    }
}

FileLogger::~FileLogger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void FileLogger::log(const std::string& event) {
    Logger::log(event);       // salvează în vector
    logFile << event << "\n"; // scrie în fișier
    logFile.flush();
}

void FileLogger::log_timed(const std::string& event){
    Logger::log_timed(event);
    logFile <<Logger::get_time()<<" "<<event << "\n";
}

void FileLogger::exportTo(const std::string& path) const {
    exportLogs(path);
}