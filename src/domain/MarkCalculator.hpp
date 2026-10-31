#pragma once
#include "domain/LatePenaltyStrategy.hpp"
#include "domain/Types.hpp"

namespace lms {

// Context of the Strategy pattern: works out the mark a student actually receives.
class MarkCalculator {
public:
    explicit MarkCalculator(const LatePenaltyStrategy& strategy) : strategy_(strategy) {}

    int daysLate(TimePoint submittedAt, TimePoint effectiveDeadline) const;
    // raw mark minus the penalty (a share of maxMark), never below zero.
    double finalMark(double rawMark, double maxMark, int daysLate) const;

private:
    const LatePenaltyStrategy& strategy_;
};

}  // namespace lms
