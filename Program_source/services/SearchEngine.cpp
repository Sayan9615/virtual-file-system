#include "SearchEngine.h"

#include <algorithm>
#include <cctype>

namespace {
std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}
}

std::vector<SearchEngine::SearchResult>
SearchEngine::search(Folder* root, const std::string& query) const
{
    std::vector<SearchResult> results;
    if (!root || query.empty()) {
        return results;
    }

    searchInFolder(root, toLower(query), results);
    return results;
}

void SearchEngine::searchInFolder(
    Folder* folder,
    const std::string& normalizedQuery,
    std::vector<SearchResult>& results) const
{
    for (const auto& child : folder->getChildren()) {
        if (toLower(child->getName()).find(normalizedQuery) != std::string::npos) {
            results.push_back({
                folder->getAbsolutePath() + "/" + child->getName(),
                child
            });
        }

        if (child->isFolder()) {
            searchInFolder(
                static_cast<Folder*>(child.get()),
                normalizedQuery,
                results);
        }
    }
}
