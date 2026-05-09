#pragma once
#include "Logger.h"
#include <string>

class TimestampedLogger : public Logger {
public:
    explicit TimestampedLogger(const std::string& filename);
    void log(const std::string& event) override;
    void log_timed(const std::string& event) override;
private:
    std::string m_filename;
};
