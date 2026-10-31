#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "domain/Errors.hpp"
#include "domain/MarkCalculator.hpp"
#include "domain/TieredDailyPenalty.hpp"

using namespace lms;
using Catch::Approx;

TEST_CASE("Tiered penalty: 10% per day for 3 days, then zero", "[penalty][boundary][unit]") {
    TieredDailyPenalty p;
    REQUIRE(p.penaltyFraction(0) == Approx(0.0));
    REQUIRE(p.penaltyFraction(1) == Approx(0.1));
    REQUIRE(p.penaltyFraction(2) == Approx(0.2));
    REQUIRE(p.penaltyFraction(3) == Approx(0.3));
    REQUIRE(p.penaltyFraction(4) == Approx(1.0));
    REQUIRE(p.penaltyFraction(30) == Approx(1.0));
}

TEST_CASE("Tiered penalty rejects nonsense", "[penalty][unit]") {
    TieredDailyPenalty p;
    REQUIRE_THROWS_AS(p.penaltyFraction(-1), ValidationError);
    REQUIRE_THROWS_AS(TieredDailyPenalty(0.0, 3), ValidationError);
    REQUIRE_THROWS_AS(TieredDailyPenalty(1.5, 3), ValidationError);
    REQUIRE_THROWS_AS(TieredDailyPenalty(0.1, 0), ValidationError);
}

TEST_CASE("Strategies are interchangeable", "[penalty][unit]") {
    NoPenalty none;
    REQUIRE(none.penaltyFraction(5) == Approx(0.0));
    TieredDailyPenalty tiered;
    MarkCalculator withNone(none);
    MarkCalculator withTiered(tiered);
    REQUIRE(withNone.finalMark(40, 50, 3) == Approx(40.0));
    REQUIRE(withTiered.finalMark(40, 50, 3) == Approx(25.0));
}

TEST_CASE("Final mark: penalty is a share of the maximum mark", "[penalty][unit]") {
    TieredDailyPenalty tiered;
    MarkCalculator calc(tiered);
    REQUIRE(calc.finalMark(40, 50, 0) == Approx(40.0));   // on time, no change
    REQUIRE(calc.finalMark(40, 50, 1) == Approx(35.0));   // 10% of 50 = 5
    REQUIRE(calc.finalMark(40, 50, 2) == Approx(30.0));
    REQUIRE(calc.finalMark(40, 50, 3) == Approx(25.0));
    REQUIRE(calc.finalMark(40, 50, 4) == Approx(0.0));    // more than 3 days: zero
    REQUIRE(calc.finalMark(3, 50, 1) == Approx(0.0));     // never below zero
    REQUIRE(calc.finalMark(50, 50, 0) == Approx(50.0));   // full marks, on time
}

TEST_CASE("Final mark rejects invalid input", "[penalty][unit]") {
    TieredDailyPenalty tiered;
    MarkCalculator calc(tiered);
    REQUIRE_THROWS_AS(calc.finalMark(-1, 50, 0), ValidationError);
    REQUIRE_THROWS_AS(calc.finalMark(51, 50, 0), ValidationError);
    REQUIRE_THROWS_AS(calc.finalMark(10, 0, 0), ValidationError);
}
