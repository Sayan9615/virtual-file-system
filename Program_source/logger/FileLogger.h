#pragma once
#include "Logger.h"
#include "IExportable.h"
#include <fstream>

class FileLogger : public Logger, public iExportable {
public:
    explicit FileLogger(const std::string& filename);
    ~FileLogger();

    void log(const std::string& event) override;

    // iExportable
    void exportTo(const std::string& path) const override;
    std::string getFormat() const override;

private:
    std::ofstream logFile;
};