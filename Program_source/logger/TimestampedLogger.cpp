#include "TimestampedLogger.h"

TimestampedLogger::TimestampedLogger(const std::string& filename)
    : m_filename(filename) {}

void TimestampedLogger::log(const std::string& event) {
    Logger::log_timed(event);
}

void TimestampedLogger::log_timed(const std::string& event) {
    Logger::log_timed(event);
}
