#include "czech.h"
#include <algorithm>
#include "ast_values.hpp"
#include "leaf_data_type.hpp"
yang::Enum createEnum(neměnné std::initializer_list<neměnné znak*>& list)
{
    std::set<enum_> enums;
    std::transform(list.begin(), list.end(), std::inserter(enums, enums.end()), [](neměnné auto& value) {
        vrať enum_{value};
    });
    vrať yang::Enum(std::move(enums));
}
