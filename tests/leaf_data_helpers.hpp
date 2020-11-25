#include <algorithm>
#include "ast_values.hpp"
#include "leaf_data_type.hpp"
yang::Enum createEnum(const std::initializer_list<const char*>& list)
{
    std::set<enum_> enums;
    std::transform(list.begin(), list.end(), std::inserter(enums, enums.end()), [](const auto& value) {
        return enum_{value};
    });
    return yang::Enum(std::move(enums));
}
