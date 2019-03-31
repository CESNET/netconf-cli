#include "ast_handlers.hpp"
std::set<std::string> generateMissingKeyCompletionSet(std::set<std::string> keysNeeded, std::set<std::string> currentSet) {
    std::set<std::string> missingKeys                                                                                     ;
    std::set_difference(keysNeeded.begin(), keysNeeded.end(),
            currentSet.begin(), currentSet.end(),
            std::inserter(missingKeys, missingKeys.end()))                                                                ;

    std::set<std::string> res                                                                                             ;

    std::transform(missingKeys.begin(), missingKeys.end(),
                   std::inserter(res, res.end()),
                   [] (auto it) {return it + "=";})                                                                       ;
    return res                                                                                                            ;}
