-- Moderated LMS (CS4433) - SQLite schema, release 1.0
-- Third Normal Form. Derived values (days late, penalty, final mark) are NOT stored.
-- Run once at first start-up:   sqlite3 lms.db < schema.sql
-- Every connection must execute:  PRAGMA foreign_keys = ON;  (SQLite leaves it off by default)

PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;          -- readers do not block the writer on a single machine
PRAGMA user_version = 1;            -- schema version; CR1 (v1.1) will raise this

-- ---------------------------------------------------------------- department
CREATE TABLE department (
    department_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    name           TEXT NOT NULL UNIQUE CHECK (length(trim(name)) > 0)
);

-- ---------------------------------------------------------------- user
-- Single-table mapping of the User hierarchy; role is the discriminator.
CREATE TABLE user (
    user_id         INTEGER PRIMARY KEY AUTOINCREMENT,
    username        TEXT NOT NULL UNIQUE COLLATE NOCASE CHECK (length(username) BETWEEN 3 AND 32),
    password_hash   TEXT NOT NULL,                     -- libsodium Argon2id string (salt + parameters included)
    full_name       TEXT NOT NULL CHECK (length(trim(full_name)) > 0),
    role            TEXT NOT NULL CHECK (role IN ('STUDENT','LECTURER','HOD','ADMIN')),
    student_number  TEXT UNIQUE,                       -- students only
    department_id   INTEGER REFERENCES department(department_id) ON DELETE RESTRICT,  -- lecturers and HoDs only
    is_active       INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0,1)),
    created_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    CHECK ((role = 'STUDENT') = (student_number IS NOT NULL)),
    CHECK ((role IN ('LECTURER','HOD')) = (department_id IS NOT NULL))
);
-- at most one active head per department
CREATE UNIQUE INDEX ux_one_hod_per_department ON user(department_id) WHERE role = 'HOD' AND is_active = 1;
CREATE INDEX ix_user_role ON user(role);

-- ---------------------------------------------------------------- course
CREATE TABLE course (
    course_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    code           TEXT NOT NULL UNIQUE COLLATE NOCASE CHECK (length(trim(code)) > 0),
    title          TEXT NOT NULL CHECK (length(trim(title)) > 0),
    department_id  INTEGER NOT NULL REFERENCES department(department_id) ON DELETE RESTRICT,
    lecturer_id    INTEGER REFERENCES user(user_id) ON DELETE RESTRICT       -- NULL until assigned
);
CREATE INDEX ix_course_department ON course(department_id);
CREATE INDEX ix_course_lecturer   ON course(lecturer_id);

-- ---------------------------------------------------------------- enrolment (association table)
CREATE TABLE enrolment (
    student_id   INTEGER NOT NULL REFERENCES user(user_id)     ON DELETE RESTRICT,
    course_id    INTEGER NOT NULL REFERENCES course(course_id) ON DELETE RESTRICT,
    enrolled_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    PRIMARY KEY (student_id, course_id)
);
CREATE INDEX ix_enrolment_course ON enrolment(course_id);

-- ---------------------------------------------------------------- assignment
CREATE TABLE assignment (
    assignment_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    course_id      INTEGER NOT NULL REFERENCES course(course_id) ON DELETE RESTRICT,
    title          TEXT NOT NULL CHECK (length(trim(title)) > 0),
    description    TEXT NOT NULL DEFAULT '',
    deadline       TEXT NOT NULL,                       -- ISO-8601 UTC, e.g. 2026-11-20T14:00:00Z
    max_mark       REAL NOT NULL CHECK (max_mark > 0 AND max_mark <= 1000),
    created_by     INTEGER NOT NULL REFERENCES user(user_id) ON DELETE RESTRICT,
    created_at     TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    UNIQUE (course_id, title)
);
CREATE INDEX ix_assignment_course ON assignment(course_id);

-- ---------------------------------------------------------------- extension (per student, v1.0)
CREATE TABLE extension (
    extension_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id INTEGER NOT NULL REFERENCES assignment(assignment_id) ON DELETE RESTRICT,
    student_id    INTEGER NOT NULL REFERENCES user(user_id)             ON DELETE RESTRICT,
    new_deadline  TEXT NOT NULL,
    reason        TEXT NOT NULL CHECK (length(trim(reason)) > 0),
    granted_by    INTEGER NOT NULL REFERENCES user(user_id)             ON DELETE RESTRICT,
    granted_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    UNIQUE (assignment_id, student_id)
);

-- ---------------------------------------------------------------- submission
CREATE TABLE submission (
    submission_id  INTEGER PRIMARY KEY AUTOINCREMENT,
    assignment_id  INTEGER NOT NULL REFERENCES assignment(assignment_id) ON DELETE RESTRICT,
    student_id     INTEGER NOT NULL REFERENCES user(user_id)             ON DELETE RESTRICT,
    content_path   TEXT NOT NULL CHECK (length(content_path) > 0),
    submitted_at   TEXT NOT NULL,
    attempt_count  INTEGER NOT NULL DEFAULT 1 CHECK (attempt_count >= 1),
    state          TEXT NOT NULL CHECK (state IN ('SUBMITTED','LATE','MARKED','MODERATED','RELEASED')),
    raw_mark       REAL CHECK (raw_mark >= 0),
    feedback       TEXT,
    marked_by      INTEGER REFERENCES user(user_id) ON DELETE RESTRICT,
    marked_at      TEXT,
    moderated_by   INTEGER REFERENCES user(user_id) ON DELETE RESTRICT,
    moderated_at   TEXT,
    released_at    TEXT,
    UNIQUE (assignment_id, student_id),
    -- the state and the mark columns must agree
    CHECK (state IN ('SUBMITTED','LATE') = (raw_mark IS NULL)),
    CHECK (state NOT IN ('MODERATED','RELEASED') OR moderated_by IS NOT NULL),
    CHECK (state <> 'RELEASED' OR released_at IS NOT NULL)
);
CREATE INDEX ix_submission_assignment ON submission(assignment_id);
CREATE INDEX ix_submission_student    ON submission(student_id);
CREATE INDEX ix_submission_state      ON submission(state);

-- ---------------------------------------------------------------- audit_log (append-only)
CREATE TABLE audit_log (
    audit_id       INTEGER PRIMARY KEY AUTOINCREMENT,
    submission_id  INTEGER NOT NULL REFERENCES submission(submission_id) ON DELETE RESTRICT,
    changed_by     INTEGER NOT NULL REFERENCES user(user_id)             ON DELETE RESTRICT,
    changed_at     TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
    old_mark       REAL,                                 -- NULL for the first mark
    new_mark       REAL NOT NULL CHECK (new_mark >= 0),
    reason         TEXT NOT NULL CHECK (length(trim(reason)) > 0)
);
CREATE INDEX ix_audit_submission ON audit_log(submission_id);

CREATE TRIGGER trg_audit_no_update BEFORE UPDATE ON audit_log
BEGIN SELECT RAISE(ABORT, 'audit_log is append-only'); END;
CREATE TRIGGER trg_audit_no_delete BEFORE DELETE ON audit_log
BEGIN SELECT RAISE(ABORT, 'audit_log is append-only'); END;

-- ---------------------------------------------------------------- role integrity triggers
CREATE TRIGGER trg_course_lecturer_role BEFORE INSERT ON course
WHEN NEW.lecturer_id IS NOT NULL AND (SELECT role FROM user WHERE user_id = NEW.lecturer_id) <> 'LECTURER'
BEGIN SELECT RAISE(ABORT, 'course.lecturer_id must reference a LECTURER'); END;
CREATE TRIGGER trg_course_lecturer_role_upd BEFORE UPDATE OF lecturer_id ON course
WHEN NEW.lecturer_id IS NOT NULL AND (SELECT role FROM user WHERE user_id = NEW.lecturer_id) <> 'LECTURER'
BEGIN SELECT RAISE(ABORT, 'course.lecturer_id must reference a LECTURER'); END;
CREATE TRIGGER trg_enrolment_student_role BEFORE INSERT ON enrolment
WHEN (SELECT role FROM user WHERE user_id = NEW.student_id) <> 'STUDENT'
BEGIN SELECT RAISE(ABORT, 'enrolment.student_id must reference a STUDENT'); END;
CREATE TRIGGER trg_submission_student_role BEFORE INSERT ON submission
WHEN (SELECT role FROM user WHERE user_id = NEW.student_id) <> 'STUDENT'
BEGIN SELECT RAISE(ABORT, 'submission.student_id must reference a STUDENT'); END;
