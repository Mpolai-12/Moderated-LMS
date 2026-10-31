#pragma once
#include <memory>
#include <optional>
#include <set>
#include <string>

#include "domain/Types.hpp"

namespace lms {

// Plain data used to build any kind of user (from the database or from the admin menu).
struct UserFields {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string fullName;
    bool active = true;
    std::string studentNumber;          // students only
    std::optional<int> departmentId;    // lecturers and heads of department only
};

// Abstract base: every role overrides role() and permissions().
class User {
public:
    virtual ~User() = default;
    User(const User&) = delete;
    User& operator=(const User&) = delete;

    int id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& fullName() const { return fullName_; }
    const std::string& passwordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }

    virtual Role role() const = 0;
    virtual std::set<Permission> permissions() const = 0;
    bool can(Permission p) const { return permissions().count(p) > 0; }

protected:
    explicit User(const UserFields& f);

private:
    int id_;
    std::string username_;
    std::string passwordHash_;
    std::string fullName_;
    bool active_;
};

class Student final : public User {
public:
    explicit Student(const UserFields& f);
    Role role() const override { return Role::Student; }
    std::set<Permission> permissions() const override;
    const std::string& studentNumber() const { return studentNumber_; }
private:
    std::string studentNumber_;
};

class Lecturer final : public User {
public:
    explicit Lecturer(const UserFields& f);
    Role role() const override { return Role::Lecturer; }
    std::set<Permission> permissions() const override;
    int departmentId() const { return departmentId_; }
private:
    int departmentId_;
};

class HeadOfDepartment final : public User {
public:
    explicit HeadOfDepartment(const UserFields& f);
    Role role() const override { return Role::HeadOfDepartment; }
    std::set<Permission> permissions() const override;
    int departmentId() const { return departmentId_; }
private:
    int departmentId_;
};

class Administrator final : public User {
public:
    explicit Administrator(const UserFields& f) : User(f) {}
    Role role() const override { return Role::Administrator; }
    std::set<Permission> permissions() const override;
};

// Factory pattern: the only place that decides which subclass to build.
class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, const UserFields& fields);
};

}  // namespace lms
