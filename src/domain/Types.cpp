#include "domain/Types.hpp"

#include "domain/Errors.hpp"

namespace lms {

std::string toString(Role role) {
    switch (role) {
        case Role::Student: return "STUDENT";
        case Role::Lecturer: return "LECTURER";
        case Role::HeadOfDepartment: return "HOD";
        case Role::Administrator: return "ADMIN";
    }
    throw ValidationError("unknown role");
}

Role roleFromString(const std::string& text) {
    if (text == "STUDENT") return Role::Student;
    if (text == "LECTURER") return Role::Lecturer;
    if (text == "HOD") return Role::HeadOfDepartment;
    if (text == "ADMIN") return Role::Administrator;
    throw ValidationError("unknown role: " + text);
}

std::string toString(SubmissionState state) {
    switch (state) {
        case SubmissionState::Submitted: return "SUBMITTED";
        case SubmissionState::Late: return "LATE";
        case SubmissionState::Marked: return "MARKED";
        case SubmissionState::Moderated: return "MODERATED";
        case SubmissionState::Released: return "RELEASED";
    }
    throw ValidationError("unknown submission state");
}

SubmissionState stateFromString(const std::string& text) {
    if (text == "SUBMITTED") return SubmissionState::Submitted;
    if (text == "LATE") return SubmissionState::Late;
    if (text == "MARKED") return SubmissionState::Marked;
    if (text == "MODERATED") return SubmissionState::Moderated;
    if (text == "RELEASED") return SubmissionState::Released;
    throw ValidationError("unknown submission state: " + text);
}

}  // namespace lms
