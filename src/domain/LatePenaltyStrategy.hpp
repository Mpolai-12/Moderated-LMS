#pragma once

namespace lms {

// Strategy pattern: one interface, interchangeable late-penalty rules.
class LatePenaltyStrategy {
public:
    virtual ~LatePenaltyStrategy() = default;
    // Fraction of the assignment's maximum mark that is removed (0.0 to 1.0).
    // A result of 1.0 means the work scores zero.
    virtual double penaltyFraction(int daysLate) const = 0;
};

// No penalty at all (used in tests and for assignments that allow late work freely).
class NoPenalty final : public LatePenaltyStrategy {
public:
    double penaltyFraction(int) const override { return 0.0; }
};

}  // namespace lms
