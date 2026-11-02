# Moderated LMS — School Technician Installation Guide

## 1. Purpose

This guide explains how a school technician can install, build and launch the Moderated LMS.

The application is a local/offline C++17 console application. It uses SQLite for local data storage and OpenSSL for password hashing. It does not require a remote database server.

## 2. System Requirements

The computer requires:

- C++17-compatible compiler
- CMake 3.16 or later
- Ninja build system
- SQLite 3 development libraries
- OpenSSL Crypto development libraries
- Git, when cloning the source code

An internet connection may be needed to download the source code or Catch2 when building tests for the first time.

## 3. Windows Installation (MSYS2 UCRT64)

### Step 1: Install MSYS2

Install MSYS2 if it is not already installed. Open the MSYS2 UCRT64 terminal.

### Step 2: Install dependencies

Run:

    pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-git

### Step 3: Obtain the project

Clone the repository:

    git clone https://github.com/Mpolai-12/Moderated-LMS.git
    cd Moderated-LMS

If the project is already on the computer, navigate to its existing project directory instead.

### Step 4: Build the application

Run these commands from the project root:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMS_BUILD_TESTS=OFF
    cmake --build build

The executable should be created at `build/lms_app.exe`.

### Step 5: Launch the application

Run:

    ./build/lms_app.exe

Always launch from the project root so the application can locate `schema/schema.sql`.

## 4. Linux Installation (Ubuntu/Debian)

### Step 1: Install dependencies

Run:

    sudo apt update
    sudo apt install build-essential cmake ninja-build libsqlite3-dev libssl-dev git

### Step 2: Obtain the project

Run:

    git clone https://github.com/Mpolai-12/Moderated-LMS.git
    cd Moderated-LMS

If the source code is already available, navigate to its project directory instead.

### Step 3: Build the application

Run:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMS_BUILD_TESTS=OFF
    cmake --build build

### Step 4: Launch the application

Run:

    ./build/lms_app

Launch from the project root directory.

## 5. First-Time Setup

On the first launch:

1. The application reads `schema/schema.sql`.
2. It creates the local database schema.
3. It prompts for creation of the initial administrator account.
4. Set an administrator password of at least 10 characters.
5. Log in using the administrator account.

The local database is named `lms.db` and is created in the project root.

## 6. Initial Administration

The administrator should:

1. Create departments.
2. Create lecturer, HoD and student accounts.
3. Assign lecturers and HoDs to the correct departments.
4. Provide student accounts with student numbers.
5. Create courses.
6. Assign lecturers to courses.
7. Enrol students in courses.

## 7. Normal Operation

The application supports four roles:

- **Administrator:** manages departments, accounts, courses and enrolments.
- **Lecturer:** creates assignments, grants individual extensions, views submissions, awards marks and provides feedback.
- **Head of Department (HoD):** reviews departmental marks, moderates results, adjusts marks and controls mark release.
- **Student:** views assignments and deadlines, submits or resubmits work, and views released marks and feedback.

Students enter a local file path or submission reference when submitting work. The application records the reference; it does not upload or copy the submitted file.

## 8. Database and Backup Precautions

The application stores records in the local SQLite database `lms.db`.

The school technician should:

- Store backups in a secure location.
- Restrict access to the project directory and database.
- Back up the database regularly.
- Close the application before making a simple file copy.
- Protect backups because they contain school records and account information.
- Test that backups can be restored.

The database uses SQLite WAL mode. When backing up while the application is running, use a SQLite-aware backup method that accounts for WAL files rather than copying only `lms.db`.

## 9. Security and Operational Notes

- Passwords are stored as salted PBKDF2-HMAC-SHA-256 hashes using OpenSSL.
- Plaintext passwords are not stored.
- Passwords are typed into the console and may be visible while being entered.
- Use the application in a controlled school environment.
- Only authorised personnel should access the computer and database.
- The application is local/offline and has no web or mobile interface.
- There is no password-reset workflow.
- The application does not provide file uploads, automated grading or plagiarism detection.

## 10. Assignment Deadlines and Late Submissions

Deadlines must use UTC ISO-8601 format:

    YYYY-MM-DDTHH:MM:SSZ

Example:

    2026-11-20T14:00:00Z

Keep the computer's date and time correct.

Late work is accepted. The released mark applies a deduction of 10% of the available mark for each late day, up to three days. Work more than three days late receives zero. Approved individual extensions replace the effective deadline.

## 11. Troubleshooting

### CMake cannot find a compiler

Check that the correct terminal is open:

- Windows: MSYS2 UCRT64.
- Linux: a terminal with the build-essential compiler tools installed.

### SQLite or OpenSSL dependency errors

Confirm that the required development packages are installed.

### The application cannot find the schema

Run the application from the project root directory, where the `schema` folder is located.

### The application cannot create or access the database

Check that the user account running the application has permission to write to the project directory.

## 12. Installation Completion Checklist

- [ ] Required compiler and dependencies installed.
- [ ] Project source code available.
- [ ] CMake configuration completed successfully.
- [ ] Application compiled successfully.
- [ ] Application launches from the project root.
- [ ] Initial administrator account created.
- [ ] Local database `lms.db` created.
- [ ] Database backup location identified.
- [ ] Authorised personnel informed about security precautions.
