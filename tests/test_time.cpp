#include <catch2/catch_test_macros.hpp>

#include "domain/Errors.hpp"
#include "domain/Time.hpp"

using namespace lms;
using std::chrono::hours;
using std::chrono::seconds;

TEST_CASE("ISO text round-trips", "[time][unit]") {
    for (const char* s : {"2026-11-20T14:00:00Z", "1970-01-01T00:00:00Z", "2028-02-29T23:59:59Z", "2026-12-31T00:00:01Z"}) {
        REQUIRE(toIso(fromIso(s)) == s);
    }
}

TEST_CASE("Bad date text is rejected", "[time][unit]") {
    for (const char* s : {"", "tomorrow", "2026-13-01T00:00:00Z", "2026-02-30T00:00:00Z", "2027-02-29T00:00:00Z",
                          "2026-11-20 14:00:00", "2026-11-20T24:00:00Z", "2026-11-20T14:00:00"}) {
        REQUIRE_THROWS_AS(fromIso(s), ValidationError);
    }
}

TEST_CASE("Days late: boundary values around the deadline", "[time][boundary][unit]") {
    const TimePoint deadline = fromIso("2026-11-20T14:00:00Z");
    REQUIRE(daysLate(deadline - hours(48), deadline) == 0);      // early
    REQUIRE(daysLate(deadline, deadline) == 0);                  // exactly on time
    REQUIRE(daysLate(deadline + seconds(1), deadline) == 1);     // one second late is one day
    REQUIRE(daysLate(deadline + hours(24), deadline) == 1);      // exactly 1 day
    REQUIRE(daysLate(deadline + hours(24) + seconds(1), deadline) == 2);
    REQUIRE(daysLate(deadline + hours(72), deadline) == 3);      // exactly 3 days
    REQUIRE(daysLate(deadline + hours(72) + seconds(1), deadline) == 4);
}
