#include "Logger.h"
#include <fstream>
#include <sstream>
#include <chrono>

void Logger::log(const std::string& event) {
    logs.push_back(event);
}

void Logger::clear() {
    logs.clear();
}

void Logger::exportLogs(const std::string& path) const {
    std::ofstream file(path, std::ios::app);
    for(int i = 0; i < this->logs.size();i++){
        file << this->logs[i] << "\n";
    }
}

std::string Logger::get_time(){
    const auto now = std::chrono::system_clock::now();
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::ctime(&t_c);
    std::string timp = ss.str();
    timp.pop_back();
    timp = '['+ timp + ']';
    return timp;
}

void Logger::log_timed(const std::string& event){
    std::string eveniment = event;
    eveniment = Logger::get_time()+ " " + eveniment;
    this->logs.push_back(eveniment);
}