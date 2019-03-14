#pragma once

#include <catch2/catch.hpp>
#include <trompeloeil.hpp>

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

void waitForCompletionAndBitMore(const trompeloeil::sequence& seq);
