#pragma once
#include "../filesystem/Folder.h"
#include "../filesystem/File.h"
#include <vector>
#include <string>
#include <memory>
#include <regex>

class SearchEngine {
public:
    struct SearchResult {
        std::string absolutePath;
        std::shared_ptr<FileSystemEntity> entity;
        std::vector<std::string> matchingLines;
    };

    // Cauta dupa nume (suporta wildcard: *.txt, doc*)
    std::vector<SearchResult> searchByName(Folder* root, const std::string& pattern) const;

    // Cauta dupa continut in fisiere
    std::vector<SearchResult> searchByContent(Folder* root, const std::string& text) const;

    // Cauta dupa extensie (ex: "txt", ".txt")
    std::vector<SearchResult> searchByExtension(Folder* root, const std::string& ext) const;

    // Cauta dupa owner
    std::vector<SearchResult> searchByOwner(Folder* root, const std::string& owner) const;

    // Combinate
    std::vector<SearchResult> search(Folder* root, const std::string& query,
                                     bool byName = true,
                                     bool byContent = true) const;

private:
    std::regex wildcardToRegex(const std::string& pattern) const;

    void dfsName(Folder* node, const std::regex& re,
                 std::vector<SearchResult>& results) const;
    void dfsContent(Folder* node, const std::string& text,
                    std::vector<SearchResult>& results) const;
    void dfsExtension(Folder* node, const std::string& ext,
                      std::vector<SearchResult>& results) const;
    void dfsOwner(Folder* node, const std::string& owner,
                  std::vector<SearchResult>& results) const;
};