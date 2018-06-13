// https://github.com/CESNET/libyang/issues/523
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libyang/Libyang.hpp>
#pragma GCC diagnostic pop

int foo() {
    auto ctx = std::make_shared<Context>();
    return 666;
}
