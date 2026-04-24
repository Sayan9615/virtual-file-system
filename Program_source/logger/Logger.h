#pragma once
#include <vector>
#include <string>
#include "../interfaces/ILogger.h"

class Logger : public iLogger {
public:
    void log(const std::string& event) override;
    void clear() override;
    void exportLogs(const std::string& path) const override;
    void log_timed(const std::string& event) override;
    
protected:
    std::vector<std::string> logs;
    static std::string get_time();
};