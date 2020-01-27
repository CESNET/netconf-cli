#include "ast_handlers.hpp"
std::set<std::string> generateMissingKeyCompletionSet(std::set<KeyIdentifier> keysNeeded, std::set<KeyIdentifier> currentSet)
{
    std::set<KeyIdentifier> missingKeys;
    std::set_difference(keysNeeded.begin(), keysNeeded.end(),
            currentSet.begin(), currentSet.end(),
            std::inserter(missingKeys, missingKeys.end()));

    std::set<std::string> res;

    std::transform(missingKeys.begin(), missingKeys.end(),
                   std::inserter(res, res.end()),
                   [] (auto it) { return keyIdentifierToString(it) + "="; });
    return res;
}
