#include "ast_handlers.hpp"
std::set<std::string> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::map<std::string, leaf_data_> currentKeys)
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

std::string leafDataToCompletion(const leaf_data_& value)
{
    // Only string-like values need to be quoted
    if (value.type() == typeid(std::string)) {
        return escapeListKeyString(leafDataToString(value));
    }
    return leafDataToString(value);
}
