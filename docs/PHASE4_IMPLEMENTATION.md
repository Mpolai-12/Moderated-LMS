# Phase 4 Implementation Record

## Technology decisions

- **C++17** is used for the console application and domain layer.
- **SQLite** stores user accounts, departments, courses, enrolments, assignments, extensions, submissions and mark-audit entries. SQLite foreign-key enforcement is enabled on each connection.
- **OpenSSL PBKDF2-HMAC-SHA-256** derives a 256-bit password hash using a cryptographically random 16-byte salt and 210,000 iterations. The stored value contains the algorithm name, iteration count, salt and derived hash. Login uses a constant-time comparison.
- **CMake** defines the domain library, console executable and Catch2 test target.

## Object-oriented design

The domain layer defines the abstract `User` class and concrete `Student`, `Lecturer`, `HeadOfDepartment`, and `Administrator` subclasses. Each subclass supplies its role and permission set. `UserFactory` constructs the correct concrete class when an authenticated account is loaded. The application dispatches the authenticated session to the corresponding role menu and checks ownership/department constraints before sensitive operations.

## Data access and integrity

Database access is isolated behind a small SQLite wrapper and a prepared-statement RAII class in the application. All user-provided values are bound as SQL parameters. The schema enforces unique usernames, valid role names, foreign keys, positive maximum marks, valid submission states and append-only audit rows. Transactions protect the mark update and matching audit-log insert so the two operations succeed or fail together.

## Workflow summary

1. **Administrator setup:** on the first run, the schema is created and the first administrator chooses a username and password. The administrator then creates departments, users and courses, assigns lecturers, and enrols students.
2. **Assignment creation:** a lecturer can create assignments only for courses assigned to that lecturer.
3. **Submission:** a student must be enrolled in the assignment's course. The application records a submission reference, UTC timestamp and state (`SUBMITTED` or `LATE`). Resubmission is permitted before the effective deadline while the submission remains unmarked.
4. **Extensions:** a lecturer can grant/update an individual student's extension for an assignment in their course, with a reason. The extension must be later than the original deadline and the student must be enrolled in the course.
5. **Marking:** the assigned lecturer records a raw mark and required written feedback. Each mark entry is paired with an audit record containing the actor, timestamp, old mark (NULL for the first mark), new mark and reason.
6. **Moderation and release:** a HoD can review submissions from their department, optionally adjust a mark with a reason, approve moderation, and release the approved result. Students only see marks in the released state.
7. **Late penalty:** the displayed released mark deducts 10% of the maximum available mark per late day, up to three days. If more than three days late, the displayed mark is zero. An approved extension determines the effective deadline.

## Error handling and validation

The console validates numeric ranges, required text, usernames, password length, and the UTC timestamp shape. Database constraint violations and invalid workflow transitions are caught at the menu boundary and reported without terminating the application. SQLite uses a five-second busy timeout. Mark changes use a transaction.

## Requirement mapping (Phase 4)

| Assignment requirement | Implementation location |
|---|---|
| C++17 and CMake | `CMakeLists.txt`, `src/app/main.cpp`, `src/domain/` |
| Four roles, abstract User and polymorphism | `src/domain/User.hpp`, `src/domain/User.cpp`, `src/domain/UserFactory.cpp`, `login()` and role menus in `src/app/main.cpp` |
| SQLite persistence and parameterised SQL | `schema/schema.sql`, `Statement` and `Database` classes in `src/app/main.cpp` |
| Late penalties and extensions | `grantExtension()`, `submitWork()`, `viewReleasedMarks()` |
| Resubmission and lifecycle | `submitWork()` and submission state constraints in `schema/schema.sql` |
| Marking and feedback | `markSubmission()` |
| Moderation before release | `viewDepartmentMarks()`, `moderateAndRelease()` |
| Audit who/when/old/new/reason | `audit_log` table, triggers and marking/moderation transactions |
| Salted password hashes | `hashPassword()`, `verifyPassword()` using OpenSSL |
| Validation and exception handling | input helpers and menu-level exception handlers |
| Automated tests | `tests/` and CTest target in `CMakeLists.txt` |

## Limitations to resolve before final release

- The submission field stores a local path/reference, not an uploaded or copied file.
- Console password input is visible as typed; hidden input and password change/reset should be added before deployment.
- The first implementation focuses on the core use cases. Phase 5 must add integration/system/acceptance tests, report actual line coverage, and test exception and boundary paths.
- Reconcile every use case and rule against the final SRS and design diagrams before submission.
