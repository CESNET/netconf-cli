#include "czech.h"
#include "ast_handlers.hpp"
std::set<Completion> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, ListInstance currentKeys)
{
    std::set<std::string> missingKeys;

    pro (neměnné auto& key : keysNeeded) {
        když (currentKeys.find(key) == currentKeys.end()) {
            missingKeys.insert(key);
        }
    }

    std::set<Completion> res;

    std::transform(missingKeys.begin(), missingKeys.end(), std::inserter(res, res.end()), [](auto it) { vrať Completion{it + "="}; });
    vrať res;
}

std::string leafDataToCompletion(neměnné leaf_data_& value)
{
    // Only string-like values need to be quoted
    když (value.type() == typeid(std::string)) {
        vrať escapeListKeyString(leafDataToString(value));
    }
    vrať leafDataToString(value);
}

boost::optional<std::string> optModuleToOptString(neměnné boost::optional<module_> module)
{
    vrať module.flat_map([](neměnné auto& module) { vrať boost::optional<std::string>(module.m_name); });
}
