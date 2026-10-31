#include <catch2/catch_test_macros.hpp>

#include "domain/Errors.hpp"
#include "domain/User.hpp"

using namespace lms;

namespace {
UserFields fields(const std::string& name = "alice") {
    UserFields f;
    f.id = 1;
    f.username = name;
    f.passwordHash = "hash";
    f.fullName = "Alice Example";
    f.studentNumber = "S001";
    f.departmentId = 5;
    return f;
}
}  // namespace

TEST_CASE("Factory builds the right role", "[user][factory][unit]") {
    REQUIRE(UserFactory::create(Role::Student, fields())->role() == Role::Student);
    REQUIRE(UserFactory::create(Role::Lecturer, fields())->role() == Role::Lecturer);
    REQUIRE(UserFactory::create(Role::HeadOfDepartment, fields())->role() == Role::HeadOfDepartment);
    REQUIRE(UserFactory::create(Role::Administrator, fields())->role() == Role::Administrator);
}

TEST_CASE("Roles are polymorphic and get different permissions", "[user][rbac][unit]") {
    auto student = UserFactory::create(Role::Student, fields());
    auto lecturer = UserFactory::create(Role::Lecturer, fields());
    auto hod = UserFactory::create(Role::HeadOfDepartment, fields());
    auto admin = UserFactory::create(Role::Administrator, fields());

    REQUIRE(student->can(Permission::SubmitWork));
    REQUIRE(student->can(Permission::ViewReleasedMarks));
    REQUIRE_FALSE(student->can(Permission::MarkSubmissions));
    REQUIRE_FALSE(student->can(Permission::ReleaseMarks));

    REQUIRE(lecturer->can(Permission::MarkSubmissions));
    REQUIRE(lecturer->can(Permission::GrantExtension));
    REQUIRE_FALSE(lecturer->can(Permission::ReleaseMarks));      // lecturers cannot release marks
    REQUIRE_FALSE(lecturer->can(Permission::ManageUsers));

    REQUIRE(hod->can(Permission::ModerateMarks));
    REQUIRE(hod->can(Permission::ReleaseMarks));
    REQUIRE_FALSE(hod->can(Permission::MarkSubmissions));
    REQUIRE_FALSE(hod->can(Permission::SubmitWork));

    REQUIRE(admin->can(Permission::ManageUsers));
    REQUIRE(admin->can(Permission::ViewAuditLog));
    REQUIRE_FALSE(admin->can(Permission::ReleaseMarks));
    REQUIRE_FALSE(admin->can(Permission::SubmitWork));
}

TEST_CASE("Role-specific data is required", "[user][validation][unit]") {
    UserFields f = fields();
    f.studentNumber.clear();
    REQUIRE_THROWS_AS(UserFactory::create(Role::Student, f), ValidationError);
    f = fields();
    f.departmentId.reset();
    REQUIRE_THROWS_AS(UserFactory::create(Role::Lecturer, f), ValidationError);
    REQUIRE_THROWS_AS(UserFactory::create(Role::HeadOfDepartment, f), ValidationError);
    REQUIRE_NOTHROW(UserFactory::create(Role::Administrator, f));   // admins need neither
}

TEST_CASE("Usernames and names are validated", "[user][validation][boundary][unit]") {
    REQUIRE_THROWS_AS(UserFactory::create(Role::Administrator, fields("ab")), ValidationError);                    // 2 chars
    REQUIRE_NOTHROW(UserFactory::create(Role::Administrator, fields("abc")));                                      // 3 chars
    REQUIRE_NOTHROW(UserFactory::create(Role::Administrator, fields(std::string(32, 'a'))));                       // 32 chars
    REQUIRE_THROWS_AS(UserFactory::create(Role::Administrator, fields(std::string(33, 'a'))), ValidationError);     // 33 chars
    REQUIRE_THROWS_AS(UserFactory::create(Role::Administrator, fields("bad name")), ValidationError);
    REQUIRE_THROWS_AS(UserFactory::create(Role::Administrator, fields("x'; DROP TABLE user;--")), ValidationError);
    UserFields f = fields();
    f.fullName = "   ";
    REQUIRE_THROWS_AS(UserFactory::create(Role::Administrator, f), ValidationError);
}

TEST_CASE("Role and state text forms round-trip", "[types][unit]") {
    for (Role r : {Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator}) {
        REQUIRE(roleFromString(toString(r)) == r);
    }
    for (SubmissionState s : {SubmissionState::Submitted, SubmissionState::Late, SubmissionState::Marked,
                              SubmissionState::Moderated, SubmissionState::Released}) {
        REQUIRE(stateFromString(toString(s)) == s);
    }
    REQUIRE_THROWS_AS(roleFromString("TEACHER"), ValidationError);
    REQUIRE_THROWS_AS(stateFromString("DELETED"), ValidationError);
}
