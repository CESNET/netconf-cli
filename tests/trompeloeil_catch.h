#pragma once

#ifdef REQUIRE
# error This file needs to be included prior to including anything from Catch.
#endif

// clang-format off

#include <ostream>
#include <thread>
#include <catch.hpp>
#include <trompeloeil.hpp>

// this is copy-paste from https://github.com/rollbear/trompeloeil/blob/master/docs/CookBook.md/#unit_test_frameworks
namespace trompeloeil {

/** @short Pass reports from the Trompeloeil mocker to Catch for processing as test failures */
template <>
struct reporter<trompeloeil::specialized>
{
  static void send(trompeloeil::severity s,
                   const char* file,
                   unsigned long line,
                   const char* msg)
  {
    std::ostringstream os;
    if (line) os << file << ':' << line << '\n';
    os << msg;
    auto failure = os.str();
    if (s == severity::fatal)
    {
      FAIL(failure);
    }
    else
    {
      CAPTURE(failure);
      CHECK(failure.empty());
    }
  }
};
}

/** @short Wait until a given sequence of expectation is matched, and then a bit more to ensure that there's silence afterwards */
void waitForCompletionAndBitMore(const trompeloeil::sequence& seq)
{
    using namespace std::literals;
    using clock = std::chrono::steady_clock;

    // We're busy-waiting a bit
    const auto waitingStep = 30ms;
    // Timeout after this much
    const auto completionTimeout = 5000ms;
    // When checking for silence afterwards, wait at least this long.
    // We'll also wait as long as it originally took to process everything.
    const auto minExtraWait = 100ms;

    auto start = clock::now();
    while (!seq.is_completed()) {
        std::this_thread::sleep_for(waitingStep);
        if (clock::now() - start > completionTimeout) {
            break;
        }
    }
    REQUIRE(seq.is_completed());
    auto duration = std::chrono::duration<double>(clock::now() - start);
    std::this_thread::sleep_for(std::max(duration, decltype(duration)(minExtraWait)));
}
