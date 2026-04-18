#pragma once
#include <string>
#include <vector>

  class iShareable
   {

    public:
     virtual ~iShareable() = default;
    
     //ofer acces
     virtual void share(const std::string& username) = 0;
     
     //elimin accesul
     virtual void revokeAccess(const std::string& username)=0;

     //returnez un vector cu utilizatorii care au acces
     virtual std::vector<std::string> getSharedWith() const = 0;
   };