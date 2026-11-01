#include <catch2/catch_test_macros.hpp>
#include <sqlite3.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#ifndef LMS_SCHEMA_PATH
#error "LMS_SCHEMA_PATH has not been defined"
#endif

class TestDatabase {
public:
    TestDatabase() {
        if (sqlite3_open(":memory:", &db_) != SQLITE_OK) {
            throw std::runtime_error("Could not open in-memory SQLite database");
        }

        // Foreign keys must be enabled on every SQLite connection.
        if (execute("PRAGMA foreign_keys = ON;") != SQLITE_OK) {
            throw std::runtime_error("Could not enable foreign keys");
        }

        std::ifstream file(LMS_SCHEMA_PATH);
        if (!file) {
            throw std::runtime_error("Could not open schema/schema.sql");
        }

        const std::string schema(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        if (execute(schema) != SQLITE_OK) {
            throw std::runtime_error("Could not create database schema");
        }
    }

    ~TestDatabase() {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
    }

    TestDatabase(const TestDatabase&) = delete;
    TestDatabase& operator=(const TestDatabase&) = delete;

    int execute(const std::string& sql) {
        char* error_message = nullptr;
        const int result = sqlite3_exec(
            db_, sql.c_str(), nullptr, nullptr, &error_message);

        if (error_message != nullptr) {
            sqlite3_free(error_message);
        }

        return result;
    }

    sqlite3* handle() const {
        return db_;
    }

    int last_id() const {
        return static_cast<int>(sqlite3_last_insert_rowid(db_));
    }

private:
    sqlite3* db_ = nullptr;
};

// IT01: Department and lecturer can be created with a valid relationship.
TEST_CASE("IT01 - Department and lecturer relationship is stored") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO department(name) VALUES('Computer Science');"
    ) == SQLITE_OK);

    const int department_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, department_id) "
        "VALUES('lecturer1', 'hash123', 'Test Lecturer', "
        "'LECTURER', 1);"
    ) == SQLITE_OK);

    CHECK(department_id == 1);

    CHECK(sqlite3_errcode(db.handle()) == SQLITE_OK);
}

// IT02: Username uniqueness is case-insensitive.
TEST_CASE("IT02 - Duplicate usernames are rejected") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('student1', 'hash123', 'Student One', "
        "'STUDENT', 'ST001');"
    ) == SQLITE_OK);

    const int result = db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('STUDENT1', 'hash456', 'Student Two', "
        "'STUDENT', 'ST002');"
    );

    CHECK(result == SQLITE_CONSTRAINT);
}

// IT03: A course cannot reference a department that does not exist.
TEST_CASE("IT03 - Invalid course department is rejected") {
    TestDatabase db;

    const int result = db.execute(
        "INSERT INTO course(code, title, department_id) "
        "VALUES('CS101', 'Introduction to Computing', 999);"
    );

    CHECK(result == SQLITE_CONSTRAINT);
}

// IT04: Only a lecturer can be assigned to a course.
TEST_CASE("IT04 - Course lecturer must have lecturer role") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO department(name) VALUES('Computing');"
    ) == SQLITE_OK);

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, department_id) "
        "VALUES('lecturer1', 'hash', 'Lecturer One', "
        "'LECTURER', 1);"
    ) == SQLITE_OK);

    const int lecturer_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('student1', 'hash', 'Student One', "
        "'STUDENT', 'ST001');"
    ) == SQLITE_OK);

    const int student_id = db.last_id();

    CHECK(db.execute(
        "INSERT INTO course(code, title, department_id, lecturer_id) "
        "VALUES('CS101', 'Programming', 1, " +
        std::to_string(lecturer_id) + ");"
    ) == SQLITE_OK);

    const int invalid_result = db.execute(
        "INSERT INTO course(code, title, department_id, lecturer_id) "
        "VALUES('CS102', 'Databases', 1, " +
        std::to_string(student_id) + ");"
    );

    CHECK(invalid_result == SQLITE_CONSTRAINT);
}

// IT05: Only students can enrol in courses.
TEST_CASE("IT05 - Enrolment accepts students and rejects lecturers") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO department(name) VALUES('Computing');"
    ) == SQLITE_OK);

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('student1', 'hash', 'Student One', "
        "'STUDENT', 'ST001');"
    ) == SQLITE_OK);

    const int student_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, department_id) "
        "VALUES('lecturer1', 'hash', 'Lecturer One', "
        "'LECTURER', 1);"
    ) == SQLITE_OK);

    const int lecturer_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO course(code, title, department_id) "
        "VALUES('CS101', 'Programming', 1);"
    ) == SQLITE_OK);

    const int course_id = db.last_id();

    CHECK(db.execute(
        "INSERT INTO enrolment(student_id, course_id) VALUES(" +
        std::to_string(student_id) + ", " +
        std::to_string(course_id) + ");"
    ) == SQLITE_OK);

    const int invalid_result = db.execute(
        "INSERT INTO enrolment(student_id, course_id) VALUES(" +
        std::to_string(lecturer_id) + ", " +
        std::to_string(course_id) + ");"
    );

    CHECK(invalid_result == SQLITE_CONSTRAINT);
}

// IT06: Only students can submit assignments.
TEST_CASE("IT06 - Submission rejects a lecturer as the student") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO department(name) VALUES('Computing');"
    ) == SQLITE_OK);

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, department_id) "
        "VALUES('lecturer1', 'hash', 'Lecturer One', "
        "'LECTURER', 1);"
    ) == SQLITE_OK);

    const int lecturer_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('student1', 'hash', 'Student One', "
        "'STUDENT', 'ST001');"
    ) == SQLITE_OK);

    REQUIRE(db.execute(
        "INSERT INTO course(code, title, department_id) "
        "VALUES('CS101', 'Programming', 1);"
    ) == SQLITE_OK);

    const int course_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO assignment "
        "(course_id, title, deadline, max_mark, created_by) "
        "VALUES(" + std::to_string(course_id) +
        ", 'Assignment One', '2026-11-10 23:59:00', 100, " +
        std::to_string(lecturer_id) + ");"
    ) == SQLITE_OK);

    const int assignment_id = db.last_id();

    const int invalid_result = db.execute(
        "INSERT INTO submission "
        "(assignment_id, student_id, content_path, submitted_at, state) "
        "VALUES(" + std::to_string(assignment_id) + ", " +
        std::to_string(lecturer_id) +
        ", 'uploads/test.pdf', '2026-11-09 10:00:00', 'SUBMITTED');"
    );

    CHECK(invalid_result == SQLITE_CONSTRAINT);
}

// IT07: Audit records cannot be updated or deleted.
TEST_CASE("IT07 - Audit log is append-only") {
    TestDatabase db;

    REQUIRE(db.execute(
        "INSERT INTO department(name) VALUES('Computing');"
    ) == SQLITE_OK);

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, department_id) "
        "VALUES('lecturer1', 'hash', 'Lecturer One', "
        "'LECTURER', 1);"
    ) == SQLITE_OK);

    const int lecturer_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO \"user\" "
        "(username, password_hash, full_name, role, student_number) "
        "VALUES('student1', 'hash', 'Student One', "
        "'STUDENT', 'ST001');"
    ) == SQLITE_OK);

    const int student_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO course(code, title, department_id) "
        "VALUES('CS101', 'Programming', 1);"
    ) == SQLITE_OK);

    const int course_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO assignment "
        "(course_id, title, deadline, max_mark, created_by) "
        "VALUES(" + std::to_string(course_id) +
        ", 'Assignment One', '2026-11-10 23:59:00', 100, " +
        std::to_string(lecturer_id) + ");"
    ) == SQLITE_OK);

    const int assignment_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO submission "
        "(assignment_id, student_id, content_path, submitted_at, state) "
        "VALUES(" + std::to_string(assignment_id) + ", " +
        std::to_string(student_id) +
        ", 'uploads/test.pdf', '2026-11-09 10:00:00', 'SUBMITTED');"
    ) == SQLITE_OK);

    const int submission_id = db.last_id();

    REQUIRE(db.execute(
        "INSERT INTO audit_log "
        "(submission_id, changed_by, old_mark, new_mark, reason) "
        "VALUES(" + std::to_string(submission_id) + ", " +
        std::to_string(lecturer_id) +
        ", NULL, 75, 'Initial mark');"
    ) == SQLITE_OK);

    CHECK(db.execute(
        "UPDATE audit_log SET new_mark = 80 WHERE audit_id = 1;"
    ) == SQLITE_CONSTRAINT);

    CHECK(db.execute(
        "DELETE FROM audit_log WHERE audit_id = 1;"
    ) == SQLITE_CONSTRAINT);

    sqlite3_stmt* statement = nullptr;

    REQUIRE(sqlite3_prepare_v2(
        db.handle(),
        "SELECT COUNT(*) FROM audit_log;",
        -1,
        &statement,
        nullptr) == SQLITE_OK);

    REQUIRE(sqlite3_step(statement) == SQLITE_ROW);
    CHECK(sqlite3_column_int(statement, 0) == 1);

    sqlite3_finalize(statement);
}