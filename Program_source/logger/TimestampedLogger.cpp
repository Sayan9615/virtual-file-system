#include "TimestampedLogger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

TimestampedLogger::TimestampedLogger(const std::string& filename)
    : FileLogger(filename) {}

void TimestampedLogger::log(const std::string& event) {
    FileLogger::log("[" + getCurrentTime() + "] " + event);
}

std::string TimestampedLogger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}