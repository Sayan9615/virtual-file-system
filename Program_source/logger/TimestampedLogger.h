#pragma once
#include "FileLogger.h"

class TimestampedLogger : public FileLogger {
public:
    explicit TimestampedLogger(const std::string& filename);
    void log(const std::string& event) override;

private:
    std::string getCurrentTime() const;
};