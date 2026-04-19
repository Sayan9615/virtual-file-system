#include "Logger.h"
#include <fstream>

void Logger::log(const std::string& event) {
    logs.push_back(event);
}

void Logger::clear() {
    logs.clear();
}

void Logger::exportLogs(const std::string& path) const {
    std::ofstream file(path, std::ios::app);
    for (const auto& entry : logs) {
        file << entry << "\n";
    }
}