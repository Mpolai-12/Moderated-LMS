#include "domain/User.hpp"

#include "domain/Errors.hpp"

namespace lms {

std::unique_ptr<User> UserFactory::create(Role role, const UserFields& fields) {
    switch (role) {
        case Role::Student: return std::make_unique<Student>(fields);
        case Role::Lecturer: return std::make_unique<Lecturer>(fields);
        case Role::HeadOfDepartment: return std::make_unique<HeadOfDepartment>(fields);
        case Role::Administrator: return std::make_unique<Administrator>(fields);
    }
    throw ValidationError("unknown role");
}

}  // namespace lms
