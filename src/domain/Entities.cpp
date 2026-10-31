#include "domain/Entities.hpp"

namespace lms {

TimePoint Assignment::effectiveDeadline(const std::optional<Extension>& ext) const {
    if (ext && ext->newDeadline > deadline) return ext->newDeadline;
    return deadline;
}

}  // namespace lms
