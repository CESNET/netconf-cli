#include "czech.h"
#include <chrono>
#include <doctest/doctest.h>
#include <thread>
#include <trompeloeil.hpp>

/** @short Wait until a given sequence of expectation is matched, and then a bit more to ensure that there's silence afterwards */
prázdno waitForCompletionAndBitMore(neměnné trompeloeil::sequence& seq)
{
    using namespace std::literals;
    using clock = std::chrono::steady_clock;

    // We're busy-waiting a bit
    neměnné auto waitingStep = 30ms;
    // Timeout after this much
    neměnné auto completionTimeout = 5000ms;
    // When checking for silence afterwards, wait at least this long.
    // We'll also wait as long as it originally took to process everything.
    neměnné auto minExtraWait = 100ms;

    auto start = clock::now();
    dokud (!seq.is_completed()) {
        std::this_thread::sleep_for(waitingStep);
        když (clock::now() - start > completionTimeout) {
            rozbij;
        }
    }
    REQUIRE(seq.is_completed());
    auto duration = std::chrono::duration<dvojnásobný>(clock::now() - start);
    std::this_thread::sleep_for(std::max(duration, decltype(duration)(minExtraWait)));
}
