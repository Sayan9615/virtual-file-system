#include "SearchEngine.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include <sstream>
#include <algorithm>

// ── helpers ───────────────────────────────────────────────────────────────────

std::regex SearchEngine::wildcardToRegex(const std::string& pattern) const {
    std::string re;
    for (char c : pattern) {
        switch (c) {
            case '*': re += ".*"; break;
            case '?': re += ".";  break;
            case '.': re += "\\."; break;
            default:  re += c;    break;
        }
    }
    return std::regex(re, std::regex::icase);
}

// ── DFS-uri ───────────────────────────────────────────────────────────────────

void SearchEngine::dfsName(Folder* node, const std::regex& re,
                            std::vector<SearchResult>& results) const {
    for (const auto& child : node->getChildren()) {
        if (std::regex_search(child->getName(), re)) {
            results.push_back({
                node->getAbsolutePath() + "/" + child->getName(),
                child,
                {}
            });
        }
        if (child->isFolder())
            dfsName(static_cast<Folder*>(child.get()), re, results);
    }
}

void SearchEngine::dfsContent(Folder* node, const std::string& text,
                               std::vector<SearchResult>& results) const {
    for (const auto& child : node->getChildren()) {
        if (!child->isFolder()) {
            // folosim search() din iSearchable - deja implementat in TextFile/BinaryFile
            auto* searchable = dynamic_cast<iSearchable*>(child.get());
            if (searchable && searchable->contains(text)) {
                auto lines = searchable->search(text);
                results.push_back({
                    node->getAbsolutePath() + "/" + child->getName(),
                    child,
                    lines
                });
            }
        } else {
            dfsContent(static_cast<Folder*>(child.get()), text, results);
        }
    }
}

void SearchEngine::dfsExtension(Folder* node, const std::string& ext,
                                 std::vector<SearchResult>& results) const {
    for (const auto& child : node->getChildren()) {
        if (!child->isFolder()) {
            auto* file = dynamic_cast<File*>(child.get());
            if (file) {
                std::string fileExt = file->getExtension();
                std::string queryExt = ext;
                // normalizam: scoatem punctul
                if (!fileExt.empty()  && fileExt[0]  == '.') fileExt  = fileExt.substr(1);
                if (!queryExt.empty() && queryExt[0] == '.') queryExt = queryExt.substr(1);

                if (fileExt == queryExt)
                    results.push_back({
                        node->getAbsolutePath() + "/" + child->getName(),
                        child,
                        {}
                    });
            }
        } else {
            dfsExtension(static_cast<Folder*>(child.get()), ext, results);
        }
    }
}

void SearchEngine::dfsOwner(Folder* node, const std::string& owner,
                             std::vector<SearchResult>& results) const {
    for (const auto& child : node->getChildren()) {
        if (child->getOwnerUser() == owner)
            results.push_back({
                node->getAbsolutePath() + "/" + child->getName(),
                child,
                {}
            });
        if (child->isFolder())
            dfsOwner(static_cast<Folder*>(child.get()), owner, results);
    }
}

// ── API public ────────────────────────────────────────────────────────────────

std::vector<SearchEngine::SearchResult>
SearchEngine::searchByName(Folder* root, const std::string& pattern) const {
    std::vector<SearchResult> results;
    dfsName(root, wildcardToRegex(pattern), results);
    return results;
}

std::vector<SearchEngine::SearchResult>
SearchEngine::searchByContent(Folder* root, const std::string& text) const {
    std::vector<SearchResult> results;
    dfsContent(root, text, results);
    return results;
}

std::vector<SearchEngine::SearchResult>
SearchEngine::searchByExtension(Folder* root, const std::string& ext) const {
    std::vector<SearchResult> results;
    dfsExtension(root, ext, results);
    return results;
}

std::vector<SearchEngine::SearchResult>
SearchEngine::searchByOwner(Folder* root, const std::string& owner) const {
    std::vector<SearchResult> results;
    dfsOwner(root, owner, results);
    return results;
}

std::vector<SearchEngine::SearchResult>
SearchEngine::search(Folder* root, const std::string& query,
                     bool byName, bool byContent) const {
    std::vector<SearchResult> results;

    if (byName) {
        auto r = searchByName(root, query);
        results.insert(results.end(), r.begin(), r.end());
    }

    if (byContent) {
        auto r = searchByContent(root, query);
        // deduplicam dupa absolutePath
        for (auto& res : r) {
            bool duplicate = false;
            for (const auto& existing : results)
                if (existing.absolutePath == res.absolutePath) { duplicate = true; break; }
            if (!duplicate)
                results.push_back(std::move(res));
        }
    }

    return results;
}