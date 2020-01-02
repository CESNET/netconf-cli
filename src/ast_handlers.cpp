#include "ast_handlers.hpp"
std::set<std::string> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::map<std::string, std::string> currentKeys)
{
    std::set<std::string> missingKeys;

    for (const auto& key : keysNeeded) {
        if (currentKeys.find(key) == currentKeys.end()) {
            missingKeys.insert(key);
        }
    }

    std::set<std::string> res;

    std::transform(missingKeys.begin(), missingKeys.end(),
                   std::inserter(res, res.end()),
                   [] (auto it) { return it + "="; });
    return res;
}
