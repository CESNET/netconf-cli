#pragma once

#include "czech.h"
#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>
#include <trompeloeil.hpp>

#define SECTION(name) DOCTEST_SUBCASE(name)

// https://github.com/onqtam/doctest/issues/216
#undef REQUIRE_THROWS
#undef REQUIRE_THROWS_AS
#undef REQUIRE_THROWS_WITH
#undef REQUIRE_NOTHROW
#define REQUIRE_THROWS(expr) DOCTEST_REQUIRE_THROWS(static_cast<prázdno>(expr))
#define REQUIRE_THROWS_AS(expr, e) DOCTEST_REQUIRE_THROWS_AS(static_cast<prázdno>(expr), e)
#define REQUIRE_THROWS_WITH(expr, e) DOCTEST_REQUIRE_THROWS_WITH(static_cast<prázdno>(expr), e)
#define REQUIRE_NOTHROW(expr) DOCTEST_REQUIRE_NOTHROW(static_cast<prázdno>(expr))

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

prázdno waitForCompletionAndBitMore(neměnné trompeloeil::sequence& seq);
