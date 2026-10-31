#include "domain/User.hpp"

#include "domain/Errors.hpp"

namespace lms {

User::User(const UserFields& f)
    : id_(f.id), username_(f.username), passwordHash_(f.passwordHash), fullName_(f.fullName), active_(f.active) {
    if (f.username.size() < 3 || f.username.size() > 32) {
        throw ValidationError("username must be 3 to 32 characters");
    }
    for (char c : f.username) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok) throw ValidationError("username may contain only letters, digits, '.', '_' and '-'");
    }
    if (f.fullName.find_first_not_of(" \t") == std::string::npos) {
        throw ValidationError("full name must not be empty");
    }
}

Student::Student(const UserFields& f) : User(f), studentNumber_(f.studentNumber) {
    if (studentNumber_.empty()) throw ValidationError("a student needs a student number");
}

Lecturer::Lecturer(const UserFields& f) : User(f), departmentId_(0) {
    if (!f.departmentId) throw ValidationError("a lecturer needs a department");
    departmentId_ = *f.departmentId;
}

HeadOfDepartment::HeadOfDepartment(const UserFields& f) : User(f), departmentId_(0) {
    if (!f.departmentId) throw ValidationError("a head of department needs a department");
    departmentId_ = *f.departmentId;
}

std::set<Permission> Student::permissions() const {
    return {Permission::ViewOwnCourses, Permission::SubmitWork, Permission::ViewReleasedMarks};
}
std::set<Permission> Lecturer::permissions() const {
    return {Permission::ViewOwnCourses, Permission::CreateAssignment, Permission::GrantExtension, Permission::MarkSubmissions};
}
std::set<Permission> HeadOfDepartment::permissions() const {
    return {Permission::ReviewDepartmentMarks, Permission::ModerateMarks, Permission::ReleaseMarks};
}
std::set<Permission> Administrator::permissions() const {
    return {Permission::ManageUsers, Permission::ManageDepartments, Permission::ManageCourses,
            Permission::AssignLecturers, Permission::EnrolStudents, Permission::ViewAuditLog};
}

}  // namespace lms
