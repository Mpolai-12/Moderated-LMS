# Moderated Learning Management System (CS4433)

A local/offline C++17 console application for a secondary school's moderated LMS. It uses SQLite for persistent records and OpenSSL PBKDF2-HMAC-SHA-256 for salted password hashing. The application is designed around the supplied assignment brief and the accompanying domain model/schema.

## Implemented Phase 4 features

- Four roles: Administrator, Lecturer, Head of Department (HoD), and Student.
- Abstract `User` base class, role subclasses, polymorphic permissions, and a `UserFactory`.
- Administrator workflows for departments, accounts, courses, lecturer assignment, and student enrolment.
- Lecturer workflows for assignment creation, individual extensions, viewing submissions, marking, and written feedback.
- Student workflows for viewing assignments/deadlines, submitting and resubmitting work, and viewing released marks/feedback.
- HoD workflows for department mark review, moderation, mark adjustment, and controlled release.
- Late work is accepted. The released mark applies a deduction of 10% of the available mark for each late day, up to three days; work more than three days late receives zero. Approved individual extensions replace the effective deadline.
- Parameterised SQLite queries, foreign keys, transactions for mark/audit operations, append-only audit-log triggers, input validation, and exception handling.
- Passwords are stored as salted PBKDF2-HMAC-SHA-256 hashes generated through OpenSSL; plaintext passwords are not stored.
- CMake build and existing Catch2 domain unit tests.

## Requirements

- C++17 compiler
- CMake 3.16+
- SQLite 3 development package
- OpenSSL Crypto development package
- Catch2 v3 for tests (CMake downloads it if it is not installed; an internet connection is needed on the first configure if no local copy exists)

### Windows with MSYS2 UCRT64

Open the **MSYS2 UCRT64** terminal and install dependencies if needed:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-git
```

Navigate to the project root (the directory containing `CMakeLists.txt`) and build the application:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMS_BUILD_TESTS=OFF
cmake --build build
./build/lms_app.exe
```

To compile and run the Catch2 unit tests as well, use `-DLMS_BUILD_TESTS=ON` (or omit the option). On first configure, Catch2 may need to be downloaded. The CMake configuration registers the suite with CTest without running Catch2's test-discovery executable during the build step.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake ninja-build libsqlite3-dev libssl-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMS_BUILD_TESTS=OFF
cmake --build build
./build/lms_app
```

## First run

Run the application from the **project root** so it can read `schema/schema.sql`. On first launch it creates the database schema and asks you to create the initial administrator account. Use a password of at least 10 characters. The database file `lms.db` is created locally and is ignored by Git.

1. Log in as the initial administrator.
2. Create departments.
3. Create lecturer, HoD and student accounts. Lecturer/HoD accounts must be assigned to a department; student accounts need a student number.
4. Create courses, assign lecturers and enrol students.
5. Lecturers create assignments, set UTC deadlines and maximum marks, and grant individual extensions where justified.
6. Students enter a local file path or submission reference when submitting work. This console version records the reference; it does not upload or copy files.
7. Lecturers mark submissions and provide written feedback. HoDs review and moderate marks before releasing them.

## Deadline format and late penalties

Deadlines must use UTC ISO-8601 format: `YYYY-MM-DDTHH:MM:SSZ`, for example `2026-11-20T14:00:00Z`. Keep the computer clock correct. A late day is calculated as a 24-hour period rounded up. The raw mark is retained for moderation; the late penalty is applied when the released result is displayed.

## Project structure

- `src/domain/` — domain entities, submission lifecycle, role hierarchy, factory and penalty strategy.
- `src/app/main.cpp` — console menus and application workflows.
- `schema/schema.sql` — SQLite tables, constraints, indexes and audit triggers.
- `tests/` — Catch2 domain unit tests.
- `docs/CODING_STANDARD.md` — coding conventions.
- `docs/PHASE4_IMPLEMENTATION.md` — implementation decisions and requirement mapping.

## Important scope notes

This is a local console application, not a web/mobile LMS. It does not implement online payments, video conferencing, automated grading, plagiarism detection, a file upload service, or a password-reset workflow. Account passwords are typed into the console and may be visible while being entered; use it only in a controlled school environment until hidden terminal input and an account password-change flow are added. Before release, complete Phase 5 testing, Phase 6 static analysis/CI, and deployment documentation, and reconcile the implementation against the final Phase 2 SRS and Phase 3 diagrams.
