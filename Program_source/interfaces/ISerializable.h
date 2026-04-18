#pragma once
#include <string>

  class iSerializable
   {

    public:
        virtual ~iSerializable() = default;
    
        //convert pentru a salva date in db
        virtual std::string serialize() const = 0;

        //convert din db in obiect   
        virtual void deserialize(const std:: string &data) =0;
        
   };