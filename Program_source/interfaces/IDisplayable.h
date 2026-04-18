#pragma once
#include <string>

  class iDisplayable
    {

        public:
            virtual ~iDisplayable() = default;
            
            //afisare ob in Qt
            virtual void display() const =0;
            
            //numele icon ului pentru a pune in Qt
            virtual std::string getIcon() const = 0;
            

    };