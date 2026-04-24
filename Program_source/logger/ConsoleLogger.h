#pragma once
#include "Logger.h"

class ConsoleLogger : virtual public Logger {
public:
    void log(const std::string& event) override;
    void log_timed(const std::string& event) override;
};