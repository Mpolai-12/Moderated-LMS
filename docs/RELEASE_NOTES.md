# Moderated LMS — Release Notes

## Version 1.0

**Release type:** Initial deployment release
**Application:** Moderated Learning Management System (CS4433)
**Platform:** Windows and Linux
**Technology:** C++17, SQLite, OpenSSL

## 1. Release overview

Version 1.0 provides a local/offline console-based Learning Management System for a secondary school. It supports role-based administration, assignment management, student submissions, lecturer marking, and Head of Department moderation and release of marks.

The application uses SQLite for persistent local records and OpenSSL PBKDF2-HMAC-SHA-256 for password hashing.

## 2. Main features

### Administrator
- Create and manage departments.
- Create user accounts for students, lecturers, Heads of Department, and administrators.
- List user accounts.
- Create courses.
- Assign lecturers to courses.
- Enrol students in courses.
- View the audit log.

### Lecturer
- Create assignments for assigned courses.
- Set assignment descriptions, UTC deadlines, and maximum marks.
- Grant or update individual student deadline extensions.
- View submissions for assigned courses.
- Mark submissions and provide written feedback.
- Record reasons for marks and mark changes.

### Head of Department
- Review marks and feedback within the assigned department.
- Approve lecturer marks or adjust them with a recorded reason.
- Release moderated marks to students.
- View the department audit log.

### Student
- View enrolled assignments and deadlines.
- Submit and, where permitted, resubmit assignment work using a local file path or submission reference.
- View released marks and feedback.

## 3. Assessment rules

- Deadlines use UTC ISO-8601 format: `YYYY-MM-DDTHH:MM:SSZ`.
- An approved individual extension replaces the original deadline for that student.
- Late penalties deduct 10% of the assignment's maximum mark for each late day, up to three days.
- Work submitted more than three days late receives a final mark of zero.
- Late duration is rounded up by 24-hour periods.
- Lecturer marks require written feedback and an audit reason.
- Marks must be moderated by a Head of Department before release to students.

## 4. Security and data management

- Passwords are stored as salted PBKDF2-HMAC-SHA-256 hashes; plaintext passwords are not stored.
- SQLite is used for local persistent data.
- Parameterised database queries, foreign-key constraints, and transaction handling are used.
- Mark changes are recorded in the audit log.
- The database is stored locally as `lms.db`.

## 5. Build and deployment

The application can be built using CMake with a C++17 compiler.

Required dependencies:
- C++17 compiler
- CMake 3.16 or later
- SQLite 3 development package
- OpenSSL Crypto development package

Catch2 v3 is used for the test suite. Refer to `INSTALLATION_GUIDE.md` for platform-specific installation and build instructions.

## 6. Deployment documentation

The release documentation includes:
- `Deployment_diagram.png` — UML deployment diagram.
- `INSTALLATION_GUIDE.md` — installation and setup instructions for a school technician.
- `USER_MANUAL_ADMINISTRATOR.md` — administrator instructions.
- `USER_MANUAL_LECTURER.md` — lecturer instructions.
- `USER_MANUAL_HOD.md` — Head of Department instructions.
- `USER_MANUAL_STUDENT.md` — student instructions.

## 7. Known limitations

- The application is a local console program, not a web or mobile application.
- Student work is represented by a local file path or submission reference; files are not uploaded or copied by the application.
- Passwords are typed into the console and may be visible while being entered.
- A password-reset workflow is not implemented.
- Automated grading and plagiarism detection are not provided.
- Online payments and video conferencing are not included.
- The application requires access to its local database and schema files.

## 8. Deployment recommendation

Install the application on a school-controlled computer, follow the installation guide, create the initial administrator account, and configure departments, user accounts, courses, lecturer assignments, and student enrolments before normal use.

Keep regular backups of the local database and restrict access to the computer and database files to authorised personnel.

---

**Release:** v1.0
**Project:** Moderated Learning Management System (CS4433)
