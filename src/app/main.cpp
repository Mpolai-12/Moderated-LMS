#include <sqlite3.h>
#include "domain/User.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
    }
    ~Statement() { sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    void text(int i, const std::string& value) {
        if (sqlite3_bind_text(stmt_, i, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
    }
    void integer(int i, int value) {
        if (sqlite3_bind_int(stmt_, i, value) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_));
    }
    void real(int i, double value) {
        if (sqlite3_bind_double(stmt_, i, value) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_));
    }
    bool row() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error(sqlite3_errmsg(db_));
    }
    void done() {
        const int rc = sqlite3_step(stmt_);
        if (rc != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_));
    }
    int colInt(int i) const { return sqlite3_column_int(stmt_, i); }
    double colDouble(int i) const { return sqlite3_column_double(stmt_, i); }
    std::string colText(int i) const {
        const auto* p = sqlite3_column_text(stmt_, i);
        return p ? reinterpret_cast<const char*>(p) : "";
    }
    bool isNull(int i) const { return sqlite3_column_type(stmt_, i) == SQLITE_NULL; }
private:
    sqlite3* db_{};
    sqlite3_stmt* stmt_{};
};

class Database {
public:
    explicit Database(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            const std::string message = db_ ? sqlite3_errmsg(db_) : "Unable to open SQLite database";
            if (db_) sqlite3_close(db_);
            db_ = nullptr;
            throw std::runtime_error(message);
        }
        sqlite3_busy_timeout(db_, 5000);
        exec("PRAGMA foreign_keys=ON;");
        exec("PRAGMA journal_mode=WAL;");
    }
    ~Database() { if (db_) sqlite3_close(db_); }
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    sqlite3* raw() { return db_; }
    void exec(const std::string& sql) {
        char* error = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
            std::string message = error ? error : sqlite3_errmsg(db_);
            sqlite3_free(error);
            throw std::runtime_error(message);
        }
    }
    bool tableExists(const std::string& table) {
        Statement s(db_, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?");
        s.text(1, table);
        return s.row();
    }
private:
    sqlite3* db_{};
};

std::string trim(const std::string& s) {
    const auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c) != 0; });
    const auto last = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c){ return std::isspace(c) != 0; }).base();
    return first < last ? std::string(first, last) : std::string{};
}
std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return trim(value);
}
int askInt(const std::string& prompt, int minValue = 1, int maxValue = 1000000) {
    for (;;) {
        const auto s = ask(prompt);
        try {
            std::size_t used = 0;
            const long value = std::stol(s, &used);
            if (used == s.size() && value >= minValue && value <= maxValue) return static_cast<int>(value);
        } catch (...) {}
        std::cout << "Enter a whole number from " << minValue << " to " << maxValue << ".\n";
    }
}
double askDouble(const std::string& prompt, double minValue, double maxValue) {
    for (;;) {
        const auto s = ask(prompt);
        try {
            std::size_t used = 0;
            const double value = std::stod(s, &used);
            if (used == s.size() && std::isfinite(value) && value >= minValue && value <= maxValue) return value;
        } catch (...) {}
        std::cout << "Enter a number from " << minValue << " to " << maxValue << ".\n";
    }
}
std::string nowUtc() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
bool validIsoDate(const std::string& value) {
    // The system accepts UTC ISO timestamps so lexical comparisons are chronological.
    if (value.size() != 20 || value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
        value[13] != ':' || value[16] != ':' || value[19] != 'Z') return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) continue;
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    return true;
}
std::string askDate(const std::string& prompt) {
    for (;;) {
        auto value = ask(prompt);
        if (validIsoDate(value)) return value;
        std::cout << "Use UTC format YYYY-MM-DDTHH:MM:SSZ, e.g. 2026-11-20T14:00:00Z.\n";
    }
}

std::string hex(const unsigned char* bytes, std::size_t n) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < n; ++i) out << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    return out.str();
}
std::vector<unsigned char> unhex(const std::string& s) {
    if (s.size() % 2 != 0) throw std::runtime_error("Invalid password hash format");
    std::vector<unsigned char> result;
    for (std::size_t i = 0; i < s.size(); i += 2) {
        result.push_back(static_cast<unsigned char>(std::stoul(s.substr(i, 2), nullptr, 16)));
    }
    return result;
}
std::string hashPassword(const std::string& password) {
    if (password.size() < 10) throw std::runtime_error("Password must contain at least 10 characters.");
    unsigned char salt[16]{};
    unsigned char hash[32]{};
    if (RAND_bytes(salt, sizeof salt) != 1) throw std::runtime_error("Secure random salt generation failed.");
    constexpr int iterations = 210000;
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt, sizeof salt,
                          iterations, EVP_sha256(), sizeof hash, hash) != 1)
        throw std::runtime_error("Password hashing failed.");
    return "pbkdf2-sha256$" + std::to_string(iterations) + "$" + hex(salt, sizeof salt) + "$" + hex(hash, sizeof hash);
}
bool verifyPassword(const std::string& password, const std::string& encoded) {
    try {
        std::istringstream in(encoded);
        std::string algorithm, iterationsText, saltText, hashText;
        std::getline(in, algorithm, '$'); std::getline(in, iterationsText, '$');
        std::getline(in, saltText, '$'); std::getline(in, hashText, '$');
        if (algorithm != "pbkdf2-sha256") return false;
        const int iterations = std::stoi(iterationsText);
        const auto salt = unhex(saltText); const auto expected = unhex(hashText);
        std::vector<unsigned char> actual(expected.size());
        if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt.data(),
                              static_cast<int>(salt.size()), iterations, EVP_sha256(),
                              static_cast<int>(actual.size()), actual.data()) != 1) return false;
        return actual.size() == expected.size() && CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0;
    } catch (...) { return false; }
}

struct Session { int id{}; std::string username; std::string name; std::string role; int departmentId{}; std::unique_ptr<lms::User> actor; };
void initialize(Database& db) {
    if (db.tableExists("user")) return;
    std::ifstream file("schema/schema.sql", std::ios::binary);
    if (!file) throw std::runtime_error("Could not find schema/schema.sql. Run this program from the project root folder.");
    std::ostringstream contents; contents << file.rdbuf();
    db.exec(contents.str());
    std::cout << "First-time setup: create the initial system administrator.\n";
    std::string username;
    do {
        username = ask("Admin username (3-32 letters, digits, dot, underscore or hyphen): ");
        const bool valid = username.size() >= 3 && username.size() <= 32 && std::all_of(username.begin(), username.end(), [](unsigned char c) {
            return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
        });
        if (!valid) std::cout << "Invalid username format.\n";
        else break;
    } while (true);
    std::string fullName;
    do { fullName = ask("Admin full name: "); } while (fullName.empty());
    std::string password;
    for (;;) {
        password = ask("Admin password (at least 10 characters): ");
        if (password.size() >= 10 && password == ask("Confirm password: ")) break;
        std::cout << "Passwords did not match or were too short. Try again.\n";
    }
    Statement s(db.raw(), "INSERT INTO user(username,password_hash,full_name,role) VALUES(?,?,?,'ADMIN')");
    s.text(1, username); s.text(2, hashPassword(password)); s.text(3, fullName); s.done();
    std::cout << "Administrator created. Log in to continue.\n";
}
Session login(Database& db) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        const auto username = ask("Username: ");
        const auto password = ask("Password: ");
        Statement s(db.raw(), "SELECT user_id,username,full_name,role,COALESCE(department_id,0),password_hash,is_active,COALESCE(student_number,'') FROM user WHERE username=? COLLATE NOCASE");
        s.text(1, username);
        if (s.row() && s.colInt(6) == 1 && verifyPassword(password, s.colText(5))) {
            lms::UserFields fields;
            fields.id = s.colInt(0); fields.username = s.colText(1); fields.fullName = s.colText(2);
            fields.passwordHash = s.colText(5); fields.active = s.colInt(6) == 1;
            fields.studentNumber = s.colText(7);
            if (s.colInt(4) != 0) fields.departmentId = s.colInt(4);
            auto actor = lms::UserFactory::create(lms::roleFromString(s.colText(3)), fields);
            return {actor->id(), actor->username(), actor->fullName(), s.colText(3), s.colInt(4), std::move(actor)};
        }
        std::cout << "Login failed. Check credentials or contact the administrator.\n";
    }
    throw std::runtime_error("Too many failed login attempts. Restart the application to try again.");
}
void listDepartments(Database& db) {
    Statement s(db.raw(), "SELECT department_id,name FROM department ORDER BY name");
    std::cout << "\nDepartments\n";
    while (s.row()) std::cout << s.colInt(0) << ". " << s.colText(1) << '\n';
}
void listUsers(Database& db, const std::string& role = "") {
    Statement s(db.raw(), role.empty() ?
      "SELECT user_id,username,full_name,role,COALESCE(student_number,''),COALESCE(department_id,0),is_active FROM user ORDER BY user_id" :
      "SELECT user_id,username,full_name,role,COALESCE(student_number,''),COALESCE(department_id,0),is_active FROM user WHERE role=? ORDER BY user_id");
    if (!role.empty()) s.text(1, role);
    std::cout << "\nID | Username | Full name | Role | Student no. | Dept ID | Active\n";
    while (s.row()) std::cout << s.colInt(0) << " | " << s.colText(1) << " | " << s.colText(2) << " | " << s.colText(3)
        << " | " << s.colText(4) << " | " << s.colInt(5) << " | " << (s.colInt(6) ? "yes" : "no") << '\n';
}
void createDepartment(Database& db) {
    const auto name = ask("Department name: "); if (name.empty()) throw std::runtime_error("Department name is required.");
    Statement s(db.raw(), "INSERT INTO department(name) VALUES(?)"); s.text(1, name); s.done();
    std::cout << "Department created.\n";
}
void createUserSafe(Database& db) {
    std::cout << "Role: 1 Student, 2 Lecturer, 3 Head of Department, 4 Administrator\n";
    const int choice = askInt("Role: ", 1, 4);
    const std::string role = choice == 1 ? "STUDENT" : choice == 2 ? "LECTURER" : choice == 3 ? "HOD" : "ADMIN";
    const auto username = ask("Username (3-32 letters, digits, dot, underscore or hyphen): ");
    const bool validUsername = username.size() >= 3 && username.size() <= 32 && std::all_of(username.begin(), username.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
    });
    if (!validUsername) throw std::runtime_error("Username must be 3-32 characters and contain only letters, digits, '.', '_' or '-'.");
    const auto fullName = ask("Full name: "); if (fullName.empty()) throw std::runtime_error("Full name is required.");
    const auto password = ask("Initial password (at least 10 characters): ");
    const auto hashed = hashPassword(password);
    if (role == "STUDENT") {
        const auto number = ask("Student number: "); if (number.empty()) throw std::runtime_error("Student number is required.");
        Statement s(db.raw(), "INSERT INTO user(username,password_hash,full_name,role,student_number,department_id) VALUES(?,?,?,'STUDENT',?,NULL)");
        s.text(1, username); s.text(2, hashed); s.text(3, fullName); s.text(4, number); s.done();
    } else if (role == "LECTURER" || role == "HOD") {
        listDepartments(db); const int dept = askInt("Department ID: ");
        Statement s(db.raw(), "INSERT INTO user(username,password_hash,full_name,role,student_number,department_id) VALUES(?,?,?,? ,NULL,?)");
        s.text(1, username); s.text(2, hashed); s.text(3, fullName); s.text(4, role); s.integer(5, dept); s.done();
    } else {
        Statement s(db.raw(), "INSERT INTO user(username,password_hash,full_name,role,student_number,department_id) VALUES(?,?,?,'ADMIN',NULL,NULL)");
        s.text(1, username); s.text(2, hashed); s.text(3, fullName); s.done();
    }
    std::cout << "User created. Give the initial password to the user securely and ask them to change it.\n";
}
void createCourse(Database& db) {
    const auto code = ask("Course code: "); const auto title = ask("Course title: ");
    if (code.empty() || title.empty()) throw std::runtime_error("Course code and title are required.");
    listDepartments(db); const int dept = askInt("Department ID: ");
    Statement s(db.raw(), "INSERT INTO course(code,title,department_id) VALUES(?,?,?)");
    s.text(1, code); s.text(2, title); s.integer(3, dept); s.done();
    std::cout << "Course created.\n";
}
void assignLecturer(Database& db) {
    Statement courses(db.raw(), "SELECT course_id,code,title FROM course ORDER BY code");
    std::cout << "Courses\n"; while (courses.row()) std::cout << courses.colInt(0) << ". " << courses.colText(1) << " - " << courses.colText(2) << '\n';
    const int course = askInt("Course ID: "); listUsers(db, "LECTURER"); const int lecturer = askInt("Lecturer user ID: ");
    Statement s(db.raw(), "UPDATE course SET lecturer_id=? WHERE course_id=?"); s.integer(1, lecturer); s.integer(2, course); s.done();
    if (sqlite3_changes(db.raw()) == 0) throw std::runtime_error("Course was not found.");
    std::cout << "Lecturer assigned.\n";
}
void enrolStudent(Database& db) {
    Statement courses(db.raw(), "SELECT course_id,code,title FROM course ORDER BY code");
    std::cout << "Courses\n"; while (courses.row()) std::cout << courses.colInt(0) << ". " << courses.colText(1) << " - " << courses.colText(2) << '\n';
    const int course = askInt("Course ID: "); listUsers(db, "STUDENT"); const int student = askInt("Student user ID: ");
    Statement s(db.raw(), "INSERT INTO enrolment(student_id,course_id) VALUES(?,?)"); s.integer(1, student); s.integer(2, course); s.done();
    std::cout << "Student enrolled.\n";
}
void createAssignment(Database& db, const Session& user) {
    Statement courses(db.raw(), "SELECT course_id,code,title FROM course WHERE lecturer_id=? ORDER BY code"); courses.integer(1, user.id);
    std::cout << "Your courses\n"; bool any = false;
    while (courses.row()) { any = true; std::cout << courses.colInt(0) << ". " << courses.colText(1) << " - " << courses.colText(2) << '\n'; }
    if (!any) { std::cout << "No courses are assigned to you. Contact the administrator.\n"; return; }
    const int course = askInt("Course ID: ");
    Statement ownership(db.raw(), "SELECT 1 FROM course WHERE course_id=? AND lecturer_id=?");
    ownership.integer(1, course); ownership.integer(2, user.id);
    if (!ownership.row()) throw std::runtime_error("You may only create assignments for your assigned courses.");
    const auto title = ask("Assignment title: ");
    if (title.empty()) throw std::runtime_error("Assignment title is required.");
    const auto desc = ask("Description: "); const auto deadline = askDate("Deadline (UTC): ");
    const double maxMark = askDouble("Maximum mark: ", 0.01, 1000.0);
    Statement s(db.raw(), "INSERT INTO assignment(course_id,title,description,deadline,max_mark,created_by) VALUES(?,?,?,?,?,?)");
    s.integer(1, course); s.text(2, title); s.text(3, desc); s.text(4, deadline); s.real(5, maxMark); s.integer(6, user.id); s.done();
    std::cout << "Assignment created.\n";
}
void grantExtension(Database& db, const Session& user) {
    Statement s(db.raw(), "SELECT a.assignment_id,a.title,c.code FROM assignment a JOIN course c ON c.course_id=a.course_id WHERE c.lecturer_id=? ORDER BY a.assignment_id");
    s.integer(1, user.id); std::cout << "Assignments\n"; while (s.row()) std::cout << s.colInt(0) << ". " << s.colText(2) << " - " << s.colText(1) << '\n';
    const int assignment = askInt("Assignment ID: "); listUsers(db, "STUDENT"); const int student = askInt("Student user ID: ");
    Statement ownership(db.raw(), "SELECT a.deadline,c.course_id FROM assignment a JOIN course c ON c.course_id=a.course_id WHERE a.assignment_id=? AND c.lecturer_id=?");
    ownership.integer(1, assignment); ownership.integer(2, user.id);
    if (!ownership.row()) throw std::runtime_error("You may only grant extensions for assignments in your courses.");
    const int courseId = ownership.colInt(1);
    Statement enrolled(db.raw(), "SELECT 1 FROM enrolment WHERE student_id=? AND course_id=?");
    enrolled.integer(1, student); enrolled.integer(2, courseId);
    if (!enrolled.row()) throw std::runtime_error("The selected student is not enrolled in this assignment's course.");
    const auto deadline = askDate("New deadline (UTC): ");
    if (deadline <= ownership.colText(0)) throw std::runtime_error("An extension must be later than the original deadline.");
    const auto reason = ask("Reason for extension: ");
    if (reason.empty()) throw std::runtime_error("Extension reason is required.");
    Statement q(db.raw(), "INSERT INTO extension(assignment_id,student_id,new_deadline,reason,granted_by) VALUES(?,?,?,?,?) ON CONFLICT(assignment_id,student_id) DO UPDATE SET new_deadline=excluded.new_deadline,reason=excluded.reason,granted_by=excluded.granted_by,granted_at=strftime('%Y-%m-%dT%H:%M:%SZ','now')");
    q.integer(1, assignment); q.integer(2, student); q.text(3, deadline); q.text(4, reason); q.integer(5, user.id); q.done();
    std::cout << "Individual extension saved.\n";
}
void showStudentAssignments(Database& db, int studentId) {
    Statement s(db.raw(), "SELECT a.assignment_id,c.code,a.title,a.deadline,COALESCE(e.new_deadline,a.deadline),a.max_mark,COALESCE(sub.state,'NOT SUBMITTED'),COALESCE(e.reason,'') FROM enrolment en JOIN course c ON c.course_id=en.course_id JOIN assignment a ON a.course_id=c.course_id LEFT JOIN extension e ON e.assignment_id=a.assignment_id AND e.student_id=en.student_id LEFT JOIN submission sub ON sub.assignment_id=a.assignment_id AND sub.student_id=en.student_id WHERE en.student_id=? ORDER BY a.deadline");
    s.integer(1, studentId); std::cout << "ID | Course | Assignment | Original deadline | Effective deadline | Max | State | Extension\n";
    while (s.row()) std::cout << s.colInt(0) << " | " << s.colText(1) << " | " << s.colText(2) << " | " << s.colText(3) << " | " << s.colText(4) << " | " << s.colDouble(5) << " | " << s.colText(6) << " | " << s.colText(7) << '\n';
}
void submitWork(Database& db, const Session& user) {
    showStudentAssignments(db, user.id);
    const int assignment = askInt("Assignment ID: "); const auto path = ask("Local file path / submission reference: ");
    if (path.empty()) throw std::runtime_error("Submission path/reference is required.");
    Statement info(db.raw(), "SELECT a.deadline,COALESCE(e.new_deadline,a.deadline) FROM assignment a JOIN enrolment en ON en.course_id=a.course_id LEFT JOIN extension e ON e.assignment_id=a.assignment_id AND e.student_id=en.student_id WHERE a.assignment_id=? AND en.student_id=?");
    info.integer(1, assignment); info.integer(2, user.id);
    if (!info.row()) throw std::runtime_error("Assignment not found or you are not enrolled in its course.");
    const std::string effective = info.colText(1); const std::string submitted = nowUtc();
    const std::string state = submitted <= effective ? "SUBMITTED" : "LATE";
    Statement existing(db.raw(), "SELECT submission_id,state,attempt_count FROM submission WHERE assignment_id=? AND student_id=?");
    existing.integer(1, assignment); existing.integer(2, user.id);
    if (existing.row()) {
        if (existing.colText(1) != "SUBMITTED" || submitted > effective)
            throw std::runtime_error("Resubmission is only allowed before the effective deadline and before marking.");
        const int id = existing.colInt(0); const int count = existing.colInt(2) + 1;
        Statement update(db.raw(), "UPDATE submission SET content_path=?,submitted_at=?,attempt_count=?,state=? WHERE submission_id=? AND state='SUBMITTED'");
        update.text(1, path); update.text(2, submitted); update.integer(3, count); update.text(4, state); update.integer(5, id); update.done();
        std::cout << "Resubmission saved. Attempt " << count << ".\n";
    } else {
        Statement insert(db.raw(), "INSERT INTO submission(assignment_id,student_id,content_path,submitted_at,state) VALUES(?,?,?,?,?)");
        insert.integer(1, assignment); insert.integer(2, user.id); insert.text(3, path); insert.text(4, submitted); insert.text(5, state); insert.done();
        std::cout << "Submission saved with state " << state << ".\n";
    }
}
void viewReleasedMarks(Database& db, int studentId) {
    Statement s(db.raw(), "SELECT c.code,a.title,sub.raw_mark,a.max_mark,sub.feedback,sub.submitted_at,COALESCE(e.new_deadline,a.deadline) FROM submission sub JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id LEFT JOIN extension e ON e.assignment_id=a.assignment_id AND e.student_id=sub.student_id WHERE sub.student_id=? AND sub.state='RELEASED' ORDER BY a.title");
    s.integer(1, studentId); bool any = false;
    while (s.row()) {
        any = true; const std::string submitted = s.colText(5), deadline = s.colText(6); int daysLate = 0;
        if (submitted > deadline) {
            // Count calendar days from the ISO date; late duration is rounded up by 24-hour periods.
            try {
                const auto toTime = [](const std::string& x) { std::tm tm{}; std::istringstream in(x); in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); return std::mktime(&tm); };
                const double seconds = std::difftime(toTime(submitted), toTime(deadline));
                daysLate = std::max(1, static_cast<int>(std::ceil(seconds / 86400.0)));
            } catch (...) { daysLate = 1; }
        }
        const double raw = s.colDouble(2), maxMark = s.colDouble(3);
        const double finalMark = daysLate > 3 ? 0.0 : std::max(0.0, raw - (maxMark * 0.10 * daysLate));
        std::cout << "Course: " << s.colText(0) << " | Assignment: " << s.colText(1) << " | Mark: " << finalMark << "/" << maxMark
                  << " (raw " << raw << ", late days " << daysLate << ")\nFeedback: " << s.colText(4) << "\n\n";
    }
    if (!any) std::cout << "No marks have been released to you.\n";
}
void listSubmissionsForLecturer(Database& db, const Session& user) {
    Statement s(db.raw(), "SELECT sub.submission_id,a.title,u.full_name,sub.submitted_at,sub.state,sub.content_path,sub.raw_mark,a.max_mark FROM submission sub JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id JOIN user u ON u.user_id=sub.student_id WHERE c.lecturer_id=? ORDER BY a.assignment_id,u.full_name");
    s.integer(1, user.id); std::cout << "Submission ID | Assignment | Student | Submitted at | State | Path | Raw mark / Max\n";
    while (s.row()) std::cout << s.colInt(0) << " | " << s.colText(1) << " | " << s.colText(2) << " | " << s.colText(3) << " | " << s.colText(4)
        << " | " << s.colText(5) << " | " << (s.isNull(6) ? "unmarked" : std::to_string(s.colDouble(6))) << "/" << s.colDouble(7) << '\n';
}
void markSubmission(Database& db, const Session& user) {
    listSubmissionsForLecturer(db, user);
    const int id = askInt("Submission ID to mark: ");
    Statement s(db.raw(), "SELECT sub.raw_mark,a.max_mark,sub.state,c.lecturer_id FROM submission sub JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id WHERE sub.submission_id=?");
    s.integer(1, id); if (!s.row()) throw std::runtime_error("Submission not found.");
    if (s.colInt(3) != user.id) throw std::runtime_error("You may only mark submissions in your assigned courses.");
    const auto state = s.colText(2); if (state != "SUBMITTED" && state != "LATE" && state != "MARKED") throw std::runtime_error("Only submitted or previously marked work can be marked.");
    const bool oldNull = s.isNull(0); const double old = s.colDouble(0), max = s.colDouble(1);
    const double mark = askDouble("Raw mark (0 to " + std::to_string(max) + "): ", 0.0, max);
    const auto feedback = ask("Written feedback (required): "); if (feedback.empty()) throw std::runtime_error("Written feedback is required.");
    const auto reason = ask("Reason for mark / mark change: "); if (reason.empty()) throw std::runtime_error("Audit reason is required.");
    db.exec("BEGIN IMMEDIATE");
    try {
        Statement update(db.raw(), "UPDATE submission SET raw_mark=?,feedback=?,marked_by=?,marked_at=?,state='MARKED',moderated_by=NULL,moderated_at=NULL,released_at=NULL WHERE submission_id=?");
        update.real(1, mark); update.text(2, feedback); update.integer(3, user.id); update.text(4, nowUtc()); update.integer(5, id); update.done();
        if (oldNull) {
            Statement first(db.raw(), "INSERT INTO audit_log(submission_id,changed_by,old_mark,new_mark,reason) VALUES(?,?,NULL,?,?)");
            first.integer(1, id); first.integer(2, user.id); first.real(3, mark); first.text(4, reason); first.done();
        } else {
            Statement audit(db.raw(), "INSERT INTO audit_log(submission_id,changed_by,old_mark,new_mark,reason) VALUES(?,?,?,?,?)");
            audit.integer(1, id); audit.integer(2, user.id); audit.real(3, old); audit.real(4, mark); audit.text(5, reason); audit.done();
        }
        db.exec("COMMIT");
    } catch (...) { db.exec("ROLLBACK"); throw; }
    std::cout << "Mark saved; submission awaits HoD moderation.\n";
}
void viewDepartmentMarks(Database& db, const Session& user) {
    Statement s(db.raw(), "SELECT sub.submission_id,c.code,a.title,u.full_name,sub.raw_mark,a.max_mark,sub.feedback,sub.state FROM submission sub JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id JOIN user u ON u.user_id=sub.student_id WHERE c.department_id=? AND sub.state IN ('MARKED','MODERATED','RELEASED') ORDER BY c.code,a.title,u.full_name");
    s.integer(1, user.departmentId); std::cout << "Submission | Course | Assignment | Student | Raw mark / Max | State | Feedback\n";
    while (s.row()) std::cout << s.colInt(0) << " | " << s.colText(1) << " | " << s.colText(2) << " | " << s.colText(3) << " | " << s.colDouble(4) << "/" << s.colDouble(5) << " | " << s.colText(7) << " | " << s.colText(6) << '\n';
}
void moderateAndRelease(Database& db, const Session& user) {
    viewDepartmentMarks(db, user);
    const int id = askInt("Submission ID to moderate/release: ");
    Statement s(db.raw(), "SELECT sub.raw_mark,a.max_mark,sub.state,c.department_id FROM submission sub JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id WHERE sub.submission_id=?");
    s.integer(1, id); if (!s.row()) throw std::runtime_error("Submission not found.");
    if (s.colInt(3) != user.departmentId) throw std::runtime_error("You may only moderate marks in your department.");
    const auto state = s.colText(2);
    if (state == "MARKED") {
        const double old = s.colDouble(0), max = s.colDouble(1);
        std::cout << "Current raw mark: " << old << "/" << max << '\n';
        const auto adjust = ask("Change mark? (y/n): ");
        double mark = old; std::string reason = "HoD moderation approval";
        if (adjust == "y" || adjust == "Y") {
            mark = askDouble("Moderated raw mark: ", 0.0, max); reason = ask("Reason for mark adjustment: ");
            if (reason.empty()) throw std::runtime_error("Reason is required for mark changes.");
        }
        db.exec("BEGIN IMMEDIATE");
        try {
            Statement upd(db.raw(), "UPDATE submission SET raw_mark=?,state='MODERATED',moderated_by=?,moderated_at=? WHERE submission_id=? AND state='MARKED'");
            upd.real(1, mark); upd.integer(2, user.id); upd.text(3, nowUtc()); upd.integer(4, id); upd.done();
            if (mark != old) {
                Statement audit(db.raw(), "INSERT INTO audit_log(submission_id,changed_by,old_mark,new_mark,reason) VALUES(?,?,?,?,?)");
                audit.integer(1, id); audit.integer(2, user.id); audit.real(3, old); audit.real(4, mark); audit.text(5, reason); audit.done();
            }
            db.exec("COMMIT");
        } catch (...) { db.exec("ROLLBACK"); throw; }
        std::cout << "Moderation completed. Run this action again to release the mark.\n";
    } else if (state == "MODERATED") {
        if (ask("Release this approved mark to the student? (y/n): ") != "y") return;
        Statement upd(db.raw(), "UPDATE submission SET state='RELEASED',released_at=? WHERE submission_id=? AND state='MODERATED'");
        upd.text(1, nowUtc()); upd.integer(2, id); upd.done();
        std::cout << "Mark released to the student.\n";
    } else throw std::runtime_error("Only MARKED submissions can be moderated, and only MODERATED submissions can be released.");
}
void viewAudit(Database& db, const Session& user) {
    std::string sql = "SELECT al.audit_id,al.submission_id,u.username,al.changed_at,al.old_mark,al.new_mark,al.reason FROM audit_log al JOIN user u ON u.user_id=al.changed_by JOIN submission sub ON sub.submission_id=al.submission_id JOIN assignment a ON a.assignment_id=sub.assignment_id JOIN course c ON c.course_id=a.course_id";
    if (user.role == "HOD") sql += " WHERE c.department_id=?";
    sql += " ORDER BY al.audit_id DESC";
    Statement s(db.raw(), sql); if (user.role == "HOD") s.integer(1, user.departmentId);
    std::cout << "Audit ID | Submission | Changed by | At | Old mark | New mark | Reason\n";
    while (s.row()) std::cout << s.colInt(0) << " | " << s.colInt(1) << " | " << s.colText(2) << " | " << s.colText(3) << " | "
        << (s.isNull(4) ? "first mark" : std::to_string(s.colDouble(4))) << " | " << s.colDouble(5) << " | " << s.colText(6) << '\n';
}
void adminMenu(Database& db, const Session& user) {
    for (;;) {
        std::cout << "\n=== ADMINISTRATOR: " << user.name << " ===\n1 Manage departments\n2 Create user\n3 List users\n4 Create course\n5 Assign lecturer\n6 Enrol student\n7 View audit log\n0 Log out\n";
        const int c = askInt("Choose: ", 0, 7);
        try {
            if (c == 0) return;
            if (c == 1) { listDepartments(db); if (ask("Add a department? (y/n): ") == "y") createDepartment(db); }
            else if (c == 2) createUserSafe(db);
            else if (c == 3) listUsers(db);
            else if (c == 4) createCourse(db);
            else if (c == 5) assignLecturer(db);
            else if (c == 6) enrolStudent(db);
            else if (c == 7) viewAudit(db, user);
        } catch (const std::exception& e) { std::cout << "Operation failed: " << e.what() << '\n'; }
    }
}
void lecturerMenu(Database& db, const Session& user) {
    for (;;) {
        std::cout << "\n=== LECTURER: " << user.name << " ===\n1 Create assignment\n2 Grant/update individual extension\n3 View submissions\n4 Mark submission / update mark\n0 Log out\n";
        const int c = askInt("Choose: ", 0, 4);
        try { if (c == 0) return; if (c == 1) createAssignment(db, user); else if (c == 2) grantExtension(db, user); else if (c == 3) listSubmissionsForLecturer(db, user); else if (c == 4) markSubmission(db, user); }
        catch (const std::exception& e) { std::cout << "Operation failed: " << e.what() << '\n'; }
    }
}
void hodMenu(Database& db, const Session& user) {
    for (;;) {
        std::cout << "\n=== HEAD OF DEPARTMENT: " << user.name << " ===\n1 Review department marks\n2 Moderate / release marks\n3 View department audit log\n0 Log out\n";
        const int c = askInt("Choose: ", 0, 3);
        try { if (c == 0) return; if (c == 1) viewDepartmentMarks(db, user); else if (c == 2) moderateAndRelease(db, user); else if (c == 3) viewAudit(db, user); }
        catch (const std::exception& e) { std::cout << "Operation failed: " << e.what() << '\n'; }
    }
}
void studentMenu(Database& db, const Session& user) {
    for (;;) {
        std::cout << "\n=== STUDENT: " << user.name << " ===\n1 View enrolled assignments and deadlines\n2 Submit / resubmit assignment\n3 View released marks and feedback\n0 Log out\n";
        const int c = askInt("Choose: ", 0, 3);
        try { if (c == 0) return; if (c == 1) showStudentAssignments(db, user.id); else if (c == 2) submitWork(db, user); else if (c == 3) viewReleasedMarks(db, user.id); }
        catch (const std::exception& e) { std::cout << "Operation failed: " << e.what() << '\n'; }
    }
}

} // namespace

int main() {
    try {
        Database db("lms.db");
        initialize(db);
        for (;;) {
            std::cout << "\n==========================================\n Moderated Learning Management System\n==========================================\n1 Log in\n0 Exit\n";
            const int option = askInt("Choose: ", 0, 1);
            if (option == 0) break;
            try {
                const Session user = login(db);
                std::cout << "Welcome, " << user.name << " (" << user.role << ").\n";
                if (!user.actor || !user.actor->isActive()) throw std::runtime_error("Inactive accounts cannot use the system.");
                switch (user.actor->role()) {
                    case lms::Role::Administrator:
                        if (!user.actor->can(lms::Permission::ManageUsers)) throw std::runtime_error("Administrator permission missing.");
                        adminMenu(db, user); break;
                    case lms::Role::Lecturer:
                        if (!user.actor->can(lms::Permission::CreateAssignment)) throw std::runtime_error("Lecturer permission missing.");
                        lecturerMenu(db, user); break;
                    case lms::Role::HeadOfDepartment:
                        if (!user.actor->can(lms::Permission::ModerateMarks)) throw std::runtime_error("HoD permission missing.");
                        hodMenu(db, user); break;
                    case lms::Role::Student:
                        if (!user.actor->can(lms::Permission::SubmitWork)) throw std::runtime_error("Student permission missing.");
                        studentMenu(db, user); break;
                }
            } catch (const std::exception& e) { std::cout << "Login/session ended: " << e.what() << '\n'; }
        }
        std::cout << "Goodbye.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal startup error: " << e.what() << '\n';
        return 1;
    }
}
