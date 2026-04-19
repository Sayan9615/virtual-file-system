#include "SortManager.h"

std::vector<std::shared_ptr<FileSystemEntity>> SortManager::sort(std::vector<std::shared_ptr<FileSystemEntity>> entities, SortCrit criterium)
{
   switch (criterium)
   {
   case SortCrit::NAME_ASC: return sortByName(entities,true);
   case SortCrit::NAME_DESC: return sortByName(entities,false);
   case SortCrit::SIZE_ASC: return sortBySize(entities,true);
   case SortCrit::SIZE_DESC: return sortBySize(entities,false);
   case SortCrit::DATE_ASC: return sortByDate(entities,true);
   case SortCrit::DATE_DESC: return sortByDate(entities,false);
   case SortCrit::TYPE:{  std::stable_sort(entities.begin(),entities.end(),CompareByType()); return entities;}
   default: 
       return entities; 
   
   }
}

std::vector<std::shared_ptr<FileSystemEntity>> SortManager::sortByName(std::vector<std::shared_ptr<FileSystemEntity>> entities, bool ascending)
{
    if(ascending)
        std::stable_sort(entities.begin(),entities.end(),CompareByNameAsc());
    else
        std::stable_sort(entities.begin(),entities.end(),CompareByNameDesc());

    return entities;    
}

std::vector<std::shared_ptr<FileSystemEntity>> SortManager::sortBySize(std::vector<std::shared_ptr<FileSystemEntity>> entities, bool ascending)
{
    if(ascending)
        std::stable_sort(entities.begin(),entities.end(),CompareBySizeAsc());
    else
        std::stable_sort(entities.begin(),entities.end(),CompareBySizeDesc());

    return entities;  
}

std::vector<std::shared_ptr<FileSystemEntity>> SortManager::sortByDate(std::vector<std::shared_ptr<FileSystemEntity>> entities, bool ascending)
{
    if(ascending)
        std::stable_sort(entities.begin(),entities.end(),CompareByDateAsc());
    else
        std::stable_sort(entities.begin(),entities.end(),CompareByDateDesc());

    return entities;  
}
