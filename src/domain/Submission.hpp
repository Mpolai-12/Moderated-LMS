#pragma once
#include <optional>
#include <string>

#include "domain/Types.hpp"

namespace lms {

// Everything stored for a submission (matches the submission table).
struct SubmissionData {
    int id = 0;
    int assignmentId = 0;
    int studentId = 0;
    std::string contentPath;
    TimePoint submittedAt{};
    int attemptCount = 1;
    SubmissionState state = SubmissionState::Submitted;
    std::optional<double> rawMark;
    std::string feedback;
    std::optional<int> markedBy;
    std::optional<TimePoint> markedAt;
    std::optional<int> moderatedBy;
    std::optional<TimePoint> moderatedAt;
    std::optional<TimePoint> releasedAt;
};

// Owns the life-cycle rules (see the state machine diagram):
//   SUBMITTED / LATE -> MARKED -> MODERATED -> RELEASED.
// Any event that is not allowed in the current state throws InvalidTransitionError
// and leaves the submission unchanged.
class Submission {
public:
    // A first submission: SUBMITTED if on time, LATE if after the effective deadline.
    static Submission create(int assignmentId, int studentId, const std::string& contentPath,
                             TimePoint now, TimePoint effectiveDeadline);
    explicit Submission(SubmissionData data);   // used when loading from the database

    // Student replaces the file. Only allowed while SUBMITTED and before the deadline.
    void resubmit(const std::string& contentPath, TimePoint now, TimePoint effectiveDeadline);

    // Lecturer marks (or re-marks before moderation). Feedback is required.
    // Returns the previous raw mark so the caller can write the audit entry.
    std::optional<double> mark(double rawMark, const std::string& feedback, int markedBy,
                               TimePoint now, double maxMark);

    // Head of department changes the mark during moderation (state stays MARKED).
    std::optional<double> adjustMark(double rawMark, double maxMark);

    void moderate(int moderatedBy, TimePoint now);   // MARKED -> MODERATED
    void release(TimePoint now);                      // MODERATED -> RELEASED

    // After an extension is granted: LATE becomes SUBMITTED if the work now meets the
    // deadline. Does nothing once marking has started.
    void reassessLateness(TimePoint effectiveDeadline);

    void setId(int id) { data_.id = id; }
    const SubmissionData& data() const { return data_; }
    int id() const { return data_.id; }
    SubmissionState state() const { return data_.state; }

private:
    SubmissionData data_;
};

}  // namespace lms
