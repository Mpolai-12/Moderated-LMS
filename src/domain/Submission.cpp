#include "domain/Submission.hpp"

#include "domain/Errors.hpp"

namespace lms {
namespace {

void requireText(const std::string& text, const char* what) {
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw ValidationError(std::string(what) + " must not be empty");
    }
}

void requireMarkInRange(double mark, double maxMark) {
    if (mark < 0 || mark > maxMark) {
        throw ValidationError("mark must be between 0 and " + std::to_string(maxMark));
    }
}

[[noreturn]] void illegal(const char* event, SubmissionState state) {
    throw InvalidTransitionError(std::string("cannot ") + event + " a submission that is " + toString(state));
}

}  // namespace

Submission::Submission(SubmissionData data) : data_(std::move(data)) {}

Submission Submission::create(int assignmentId, int studentId, const std::string& contentPath,
                              TimePoint now, TimePoint effectiveDeadline) {
    requireText(contentPath, "file path");
    SubmissionData d;
    d.assignmentId = assignmentId;
    d.studentId = studentId;
    d.contentPath = contentPath;
    d.submittedAt = now;
    d.attemptCount = 1;
    d.state = (now <= effectiveDeadline) ? SubmissionState::Submitted : SubmissionState::Late;
    return Submission(std::move(d));
}

void Submission::resubmit(const std::string& contentPath, TimePoint now, TimePoint effectiveDeadline) {
    if (data_.state != SubmissionState::Submitted) illegal("resubmit", data_.state);
    if (now > effectiveDeadline) throw InvalidTransitionError("resubmission is closed: the deadline has passed");
    requireText(contentPath, "file path");
    data_.contentPath = contentPath;
    data_.submittedAt = now;
    ++data_.attemptCount;
}

std::optional<double> Submission::mark(double rawMark, const std::string& feedback, int markedBy,
                                       TimePoint now, double maxMark) {
    if (data_.state != SubmissionState::Submitted && data_.state != SubmissionState::Late &&
        data_.state != SubmissionState::Marked) {
        illegal("mark", data_.state);
    }
    requireMarkInRange(rawMark, maxMark);
    requireText(feedback, "feedback");
    const std::optional<double> previous = data_.rawMark;
    data_.rawMark = rawMark;
    data_.feedback = feedback;
    data_.markedBy = markedBy;
    data_.markedAt = now;
    data_.state = SubmissionState::Marked;
    return previous;
}

std::optional<double> Submission::adjustMark(double rawMark, double maxMark) {
    if (data_.state != SubmissionState::Marked) illegal("adjust the mark of", data_.state);
    requireMarkInRange(rawMark, maxMark);
    const std::optional<double> previous = data_.rawMark;
    data_.rawMark = rawMark;
    return previous;
}

void Submission::moderate(int moderatedBy, TimePoint now) {
    if (data_.state != SubmissionState::Marked) illegal("moderate", data_.state);
    data_.moderatedBy = moderatedBy;
    data_.moderatedAt = now;
    data_.state = SubmissionState::Moderated;
}

void Submission::release(TimePoint now) {
    if (data_.state != SubmissionState::Moderated) illegal("release", data_.state);
    data_.releasedAt = now;
    data_.state = SubmissionState::Released;
}

void Submission::reassessLateness(TimePoint effectiveDeadline) {
    if (data_.state == SubmissionState::Late && data_.submittedAt <= effectiveDeadline) {
        data_.state = SubmissionState::Submitted;
    }
}

}  // namespace lms
