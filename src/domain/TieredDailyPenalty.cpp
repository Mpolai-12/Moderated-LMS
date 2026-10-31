#include "domain/TieredDailyPenalty.hpp"

#include "domain/Errors.hpp"

namespace lms {

TieredDailyPenalty::TieredDailyPenalty(double ratePerDay, int maxLateDays)
    : ratePerDay_(ratePerDay), maxLateDays_(maxLateDays) {
    if (ratePerDay <= 0.0 || ratePerDay > 1.0) throw ValidationError("penalty rate must be above 0 and at most 1");
    if (maxLateDays < 1) throw ValidationError("maximum late days must be at least 1");
}

double TieredDailyPenalty::penaltyFraction(int daysLate) const {
    if (daysLate < 0) throw ValidationError("days late cannot be negative");
    if (daysLate == 0) return 0.0;
    if (daysLate > maxLateDays_) return 1.0;
    const double f = ratePerDay_ * daysLate;
    return f > 1.0 ? 1.0 : f;
}

}  // namespace lms
