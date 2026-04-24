#pragma once
#include "Logger.h"
#include "IExportable.h"
#include <fstream>

class FileLogger : virtual public Logger, public iExportable {
public:
    FileLogger(const std::string& filename);
    ~FileLogger();

    void log(const std::string& event) override;

    // iExportable
    void exportTo(const std::string& path) const override;

    void log_timed(const std::string& event) override;

private:
    std::ofstream logFile;
};