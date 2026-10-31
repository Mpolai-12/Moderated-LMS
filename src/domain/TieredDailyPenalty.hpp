#pragma once
#include "domain/LatePenaltyStrategy.hpp"

namespace lms {

// The Head of Department's rule: 10% of the maximum mark per day late, for up to
// three days; after that the work scores zero.
class TieredDailyPenalty final : public LatePenaltyStrategy {
public:
    explicit TieredDailyPenalty(double ratePerDay = 0.10, int maxLateDays = 3);
    double penaltyFraction(int daysLate) const override;
private:
    double ratePerDay_;
    int maxLateDays_;
};

}  // namespace lms
