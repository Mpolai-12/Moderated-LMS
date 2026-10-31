#pragma once
#include <optional>
#include <string>

#include "domain/Types.hpp"

namespace lms {

struct Department {
    int id = 0;
    std::string name;
};

struct Course {
    int id = 0;
    std::string code;
    std::string title;
    int departmentId = 0;
    std::optional<int> lecturerId;   // empty until the administrator assigns one
};

struct Extension {
    int id = 0;
    int assignmentId = 0;
    int studentId = 0;
    TimePoint newDeadline{};
    std::string reason;
    int grantedBy = 0;
    TimePoint grantedAt{};
};

struct Assignment {
    int id = 0;
    int courseId = 0;
    std::string title;
    std::string description;
    TimePoint deadline{};
    double maxMark = 0;
    int createdBy = 0;

    // The student's own deadline: the extension if there is one and it is later, otherwise the standard one.
    TimePoint effectiveDeadline(const std::optional<Extension>& ext) const;
    bool isValidMark(double mark) const { return mark >= 0 && mark <= maxMark; }
};

// One row of the append-only mark history: who, when, old value, new value, why.
struct AuditEntry {
    int id = 0;
    int submissionId = 0;
    int changedBy = 0;
    TimePoint changedAt{};
    std::optional<double> oldMark;   // empty for the first marking
    double newMark = 0;
    std::string reason;
};

}  // namespace lms
