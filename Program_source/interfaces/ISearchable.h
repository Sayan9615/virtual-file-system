#pragma once
#include <vector>
#include <string>


  class iSearchable
   {  

    public:
      virtual ~iSearchable() = default;
      
      //returnez un vector cu locurile unde gaseste text ul
      virtual std::vector<std::string> search(const std:: string& text) const =0;

      //verfic daca exista o anumita data
      virtual bool contains(const std::string& data) const =0;      
  };