#pragma once
#include <string>

#include "domain/Types.hpp"

namespace lms {

// ISO-8601 UTC text, e.g. "2026-11-20T14:00:00Z". Sorting these strings sorts by time.
std::string toIso(TimePoint t);
TimePoint fromIso(const std::string& text);  // throws ValidationError on bad input

// Whole days a submission is late. Any started day counts as a full day.
// Exactly on the deadline is on time (0 days); one second later is 1 day.
int daysLate(TimePoint submittedAt, TimePoint effectiveDeadline);

}  // namespace lms
