#pragma once
#include <string>

 class iExportable
    {

        public:
            virtual ~iExportable() = default;

            //export la date pe fisierul fizic de pe disc stabilit *livrabil1*
            virtual void exportTo(const std::string& path) const =0;

            //returnez extensia / formatul in care vor fi exportate datele
            virtual std::string getFormat() const =0;
           

    };