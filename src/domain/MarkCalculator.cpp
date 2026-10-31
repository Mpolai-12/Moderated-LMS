#include "domain/MarkCalculator.hpp"

#include "domain/Errors.hpp"
#include "domain/Time.hpp"

namespace lms {

int MarkCalculator::daysLate(TimePoint submittedAt, TimePoint effectiveDeadline) const {
    return lms::daysLate(submittedAt, effectiveDeadline);
}

double MarkCalculator::finalMark(double rawMark, double maxMark, int days) const {
    if (maxMark <= 0) throw ValidationError("maximum mark must be positive");
    if (rawMark < 0 || rawMark > maxMark) throw ValidationError("raw mark must be between 0 and the maximum mark");
    const double fraction = strategy_.penaltyFraction(days);
    if (fraction >= 1.0) return 0.0;
    const double result = rawMark - fraction * maxMark;
    return result < 0.0 ? 0.0 : result;
}

}  // namespace lms
