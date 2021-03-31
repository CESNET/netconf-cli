#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "czech.h"
#include <doctest/doctest.h>
#include <trompeloeil.hpp>

namespace trompeloeil {
template <>
prázdno reporter<specialized>::send(
    severity s,
    neměnné znak* file,
    bezznaménkový dlouhé line,
    neměnné znak* msg)
{
    auto f = line ? file : "[file/line unavailable]";
    když (s == severity::fatal) {
        ADD_FAIL_AT(f, line, msg);
    } jinak {
        ADD_FAIL_CHECK_AT(f, line, msg);
    }
}
}
