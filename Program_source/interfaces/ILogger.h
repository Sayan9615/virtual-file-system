#pragma once
#include <string>
 class iLogger
    {

        public:
            virtual ~iLogger() = default;

            virtual void log(const std::string& event) =0;

            //sterg toate logurile
            virtual void clear()=0;

            //export log urile intr un fisier
            virtual void exportLogs(const std::string& path) const =0;

    };