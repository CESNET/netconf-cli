#include "libyang_utils.hpp"

leaf_data_ leafValueFromValue(const libyang::S_Value& value, LY_DATA_TYPE type)
{
    using namespace std::string_literals;
    switch (type) {
    case LY_TYPE_INT8:
        return value->int8();
    case LY_TYPE_INT16:
        return value->int16();
    case LY_TYPE_INT32:
        return value->int32();
    case LY_TYPE_INT64:
        return value->int64();
    case LY_TYPE_UINT8:
        return value->uint8();
    case LY_TYPE_UINT16:
        return value->uint16();
    case LY_TYPE_UINT32:
        return value->uintu32();
    case LY_TYPE_UINT64:
        return value->uint64();
    case LY_TYPE_BOOL:
        return bool(value->bln());
    case LY_TYPE_STRING:
        return std::string(value->string());
    case LY_TYPE_ENUM:
        return enum_{std::string(value->enm()->name())};
    default: // TODO: implement all types
        return "(can't print)"s;
    }
}
