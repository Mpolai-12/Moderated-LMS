#pragma once
#include <chrono>
#include <string>

namespace lms {

using TimePoint = std::chrono::system_clock::time_point;

enum class Role { Student, Lecturer, HeadOfDepartment, Administrator };

enum class Permission {
    ViewOwnCourses, SubmitWork, ViewReleasedMarks,            // student
    CreateAssignment, GrantExtension, MarkSubmissions,        // lecturer
    ReviewDepartmentMarks, ModerateMarks, ReleaseMarks,       // head of department
    ManageUsers, ManageDepartments, ManageCourses,            // administrator
    AssignLecturers, EnrolStudents, ViewAuditLog
};

enum class SubmissionState { Submitted, Late, Marked, Moderated, Released };

// Text forms are what the database stores (see schema/schema.sql).
std::string toString(Role role);
Role roleFromString(const std::string& text);
std::string toString(SubmissionState state);
SubmissionState stateFromString(const std::string& text);

}  // namespace lms
