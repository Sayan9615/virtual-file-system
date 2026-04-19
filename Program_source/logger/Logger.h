#pragma once
#include "ILogger.h"
#include <vector>
#include <string>

class Logger : public iLogger {
public:
    void log(const std::string& event) override;
    void clear() override;
    void exportLogs(const std::string& path) const override;

protected:
    std::vector<std::string> logs;
};