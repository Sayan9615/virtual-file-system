#pragma once

#include "../filesystem/Folder.h"
#include <memory>
#include <string>
#include <vector>

class SearchEngine {
public:
    struct SearchResult {
        std::string absolutePath;
        std::shared_ptr<FileSystemEntity> entity;
    };

    // Cauta recursiv textul in numele fisierelor si folderelor.
    // Extensia face parte din nume, deci este cautata implicit.
    std::vector<SearchResult> search(Folder* root, const std::string& query) const;

private:
    void searchInFolder(Folder* folder, const std::string& normalizedQuery,
                        std::vector<SearchResult>& results) const;
};
