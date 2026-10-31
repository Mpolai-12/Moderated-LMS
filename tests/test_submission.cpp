#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "domain/Entities.hpp"
#include "domain/Errors.hpp"
#include "domain/Submission.hpp"
#include "domain/Time.hpp"

using namespace lms;
using std::chrono::hours;
using std::chrono::seconds;

namespace {
const TimePoint kDeadline = fromIso("2026-11-20T14:00:00Z");
Submission onTime() { return Submission::create(1, 7, "data/a.pdf", kDeadline - hours(2), kDeadline); }
Submission late() { return Submission::create(1, 7, "data/a.pdf", kDeadline + hours(5), kDeadline); }
}  // namespace

TEST_CASE("First submission is SUBMITTED on time and LATE after the deadline", "[submission][boundary][unit]") {
    REQUIRE(Submission::create(1, 7, "f", kDeadline, kDeadline).state() == SubmissionState::Submitted);       // exactly on time
    REQUIRE(Submission::create(1, 7, "f", kDeadline + seconds(1), kDeadline).state() == SubmissionState::Late);
    REQUIRE(onTime().data().attemptCount == 1);
}

TEST_CASE("A submission needs a file", "[submission][unit]") {
    REQUIRE_THROWS_AS(Submission::create(1, 7, "  ", kDeadline, kDeadline), ValidationError);
}

TEST_CASE("Resubmission is allowed only before the deadline", "[submission][resubmit][unit]") {
    Submission s = onTime();
    s.resubmit("data/b.pdf", kDeadline - hours(1), kDeadline);
    REQUIRE(s.data().attemptCount == 2);
    REQUIRE(s.data().contentPath == "data/b.pdf");
    REQUIRE(s.state() == SubmissionState::Submitted);

    s.resubmit("data/c.pdf", kDeadline, kDeadline);   // exactly at the deadline still counts
    REQUIRE(s.data().attemptCount == 3);

    REQUIRE_THROWS_AS(s.resubmit("data/d.pdf", kDeadline + seconds(1), kDeadline), InvalidTransitionError);
    REQUIRE(s.data().attemptCount == 3);              // unchanged after the refusal
    REQUIRE(s.data().contentPath == "data/c.pdf");
}

TEST_CASE("A LATE submission cannot be resubmitted", "[submission][resubmit][unit]") {
    Submission s = late();
    REQUIRE_THROWS_AS(s.resubmit("data/b.pdf", kDeadline + hours(6), kDeadline), InvalidTransitionError);
}

TEST_CASE("Marking needs a valid mark and written feedback", "[submission][marking][unit]") {
    Submission s = onTime();
    const TimePoint now = kDeadline + hours(30);
    REQUIRE_THROWS_AS(s.mark(-1, "ok", 2, now, 50), ValidationError);
    REQUIRE_THROWS_AS(s.mark(51, "ok", 2, now, 50), ValidationError);
    REQUIRE_THROWS_AS(s.mark(40, "", 2, now, 50), ValidationError);
    REQUIRE(s.state() == SubmissionState::Submitted);   // nothing changed

    REQUIRE_FALSE(s.mark(0, "zero is a valid mark", 2, now, 50).has_value());
    REQUIRE(s.mark(50, "full marks is valid", 2, now, 50) == 0.0);
}

TEST_CASE("Marking moves to MARKED and reports the old mark", "[submission][marking][unit]") {
    Submission s = late();
    const TimePoint now = kDeadline + hours(30);
    REQUIRE_FALSE(s.mark(40, "good work", 2, now, 50).has_value());   // first mark: no previous value
    REQUIRE(s.state() == SubmissionState::Marked);
    REQUIRE(s.data().rawMark == 40.0);
    REQUIRE(s.data().markedBy == 2);

    auto old = s.mark(45, "re-marked after review", 2, now + hours(1), 50);   // lecturer may re-mark before moderation
    REQUIRE(old.has_value());
    REQUIRE(*old == 40.0);
    REQUIRE(s.data().rawMark == 45.0);
}

TEST_CASE("Full life cycle: marked, moderated, released", "[submission][lifecycle][unit]") {
    Submission s = onTime();
    const TimePoint t = kDeadline + hours(30);
    s.mark(40, "good", 2, t, 50);
    auto old = s.adjustMark(42, 50);
    REQUIRE(*old == 40.0);
    s.moderate(3, t + hours(1));
    REQUIRE(s.state() == SubmissionState::Moderated);
    s.release(t + hours(2));
    REQUIRE(s.state() == SubmissionState::Released);
    REQUIRE(s.data().moderatedBy == 3);
    REQUIRE(s.data().releasedAt.has_value());
}

TEST_CASE("Illegal events are refused and change nothing", "[submission][lifecycle][unit]") {
    const TimePoint t = kDeadline + hours(30);
    Submission fresh = onTime();
    REQUIRE_THROWS_AS(fresh.moderate(3, t), InvalidTransitionError);        // not marked yet
    REQUIRE_THROWS_AS(fresh.release(t), InvalidTransitionError);
    REQUIRE_THROWS_AS(fresh.adjustMark(10, 50), InvalidTransitionError);
    REQUIRE(fresh.state() == SubmissionState::Submitted);

    Submission m = onTime();
    m.mark(40, "ok", 2, t, 50);
    REQUIRE_THROWS_AS(m.release(t), InvalidTransitionError);                // must be moderated first
    m.moderate(3, t);
    REQUIRE_THROWS_AS(m.mark(10, "too late", 2, t, 50), InvalidTransitionError);   // no re-marking after moderation
    REQUIRE_THROWS_AS(m.adjustMark(10, 50), InvalidTransitionError);
    m.release(t);
    REQUIRE_THROWS_AS(m.release(t), InvalidTransitionError);                // released is final
    REQUIRE_THROWS_AS(m.moderate(3, t), InvalidTransitionError);
    REQUIRE_THROWS_AS(m.resubmit("x", kDeadline - hours(1), kDeadline), InvalidTransitionError);
    REQUIRE(m.data().rawMark == 40.0);
}

TEST_CASE("An extension can turn LATE back into SUBMITTED", "[submission][extension][unit]") {
    Submission s = late();   // 5 hours after the deadline
    s.reassessLateness(kDeadline + hours(2));    // still late
    REQUIRE(s.state() == SubmissionState::Late);
    s.reassessLateness(kDeadline + hours(24));   // extension covers it
    REQUIRE(s.state() == SubmissionState::Submitted);

    Submission marked = late();
    marked.mark(30, "ok", 2, kDeadline + hours(30), 50);
    marked.reassessLateness(kDeadline + hours(100));   // no effect once marking has started
    REQUIRE(marked.state() == SubmissionState::Marked);
}

TEST_CASE("Assignment: effective deadline and mark range", "[assignment][unit]") {
    Assignment a;
    a.deadline = kDeadline;
    a.maxMark = 50;
    REQUIRE(a.effectiveDeadline(std::nullopt) == kDeadline);

    Extension e;
    e.newDeadline = kDeadline + hours(48);
    REQUIRE(a.effectiveDeadline(e) == kDeadline + hours(48));

    e.newDeadline = kDeadline - hours(1);              // an "extension" that is earlier is ignored
    REQUIRE(a.effectiveDeadline(e) == kDeadline);

    REQUIRE(a.isValidMark(0));
    REQUIRE(a.isValidMark(50));
    REQUIRE_FALSE(a.isValidMark(-0.5));
    REQUIRE_FALSE(a.isValidMark(50.5));
}
