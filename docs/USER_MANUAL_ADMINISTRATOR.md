# Moderated LMS — Administrator User Manual

## 1. Introduction

This manual explains how an Administrator uses the Moderated Learning Management System (LMS).

The Administrator manages the school's departments, user accounts, courses, lecturer assignments and student enrolments.

## 2. Logging In

1. Start the LMS application from the project root directory.
2. At the main menu, select `1 Log in`.
3. Enter your username and password.
4. After successful login, the Administrator menu appears.

The menu displays:

1. Manage departments
2. Create user
3. List users
4. Create course
5. Assign lecturer
6. Enrol student
7. View audit log
0. Log out

Enter the corresponding number to select an operation.

## 3. Managing Departments

Select `1 Manage departments`.

The application displays the existing departments and their department IDs.

To add a department:

1. Review the displayed departments.
2. Answer `y` when asked whether to add a department.
3. Enter the department name.
4. The application confirms when the department is created.

A department name is required.

Department IDs are used when creating lecturer and HoD accounts and when creating courses.

## 4. Creating User Accounts

Select `2 Create user`.

Choose the appropriate role:

- `1` — Student
- `2` — Lecturer
- `3` — Head of Department
- `4` — Administrator

The application requests the following information:

### Information required for every account

- Username: 3–32 characters.
- Username may contain letters, digits, dots, underscores and hyphens.
- Full name.
- Initial password of at least 10 characters.

### Additional information for students

Enter the student's student number when prompted.

### Additional information for lecturers and HoDs

1. The application displays the available departments.
2. Enter the appropriate Department ID.

### Administrator accounts

Administrator accounts do not require a student number or department assignment.

After successful creation, the application displays a confirmation message.

Give the initial password to the account holder securely. Passwords are typed into the console and may be visible while being entered.

## 5. Listing Users

Select `3 List users`.

The application displays the registered users with:

- User ID
- Username
- Full name
- Role
- Student number
- Department ID
- Active status

Use this information to identify the correct user ID when assigning lecturers or enrolling students.

## 6. Creating Courses

Select `4 Create course`.

The application requests:

1. Course code.
2. Course title.
3. Department ID.

The available departments are displayed so that you can select the correct department.

Course code and course title are required.

After successful creation, the application confirms that the course has been created.

## 7. Assigning Lecturers to Courses

Select `5 Assign lecturer`.

Follow these steps:

1. Review the displayed courses.
2. Enter the Course ID.
3. Review the displayed lecturer accounts.
4. Enter the Lecturer User ID.
5. The application confirms the assignment.

Ensure that you select the correct course and lecturer.

A course must have an assigned lecturer before that lecturer can create assignments for it.

## 8. Enrolling Students

Select `6 Enrol student`.

Follow these steps:

1. Review the displayed courses.
2. Enter the Course ID.
3. Review the displayed student accounts.
4. Enter the Student User ID.
5. The application confirms the enrolment.

Check the course and student IDs carefully before confirming the operation.

## 9. Viewing the Audit Log

Select `7 View audit log`.

The audit log displays recorded mark changes, including:

- Audit ID
- Submission ID
- Person who made the change
- Date and time
- Previous mark
- New mark
- Reason for the change

A first mark is displayed as `first mark` when there was no previous mark.

The audit log supports accountability and review of mark changes.

## 10. Logging Out

Select `0 Log out` to leave the Administrator menu.

The application returns to the main login menu.

Select `0 Exit` at the main menu to close the application.

## 11. Important Administrative Responsibilities

Administrators should:

- Use accurate department and course information.
- Create accounts for the correct roles.
- Assign lecturers to the correct departments and courses.
- Enrol students in the correct courses.
- Keep account credentials confidential.
- Give initial passwords to users securely.
- Restrict access to the application and its local database.
- Ensure that database backups are maintained.

The application does not currently provide a password-reset workflow. Passwords may also be visible while being entered into the console.

## 12. Common Problems

**Department not available:** Create the department before creating lecturer or HoD accounts or courses.

**Lecturer not listed:** Confirm that the lecturer account has been created with the Lecturer role.

**Student not listed:** Confirm that the student account has been created with the Student role.

**Course not listed:** Create the course before assigning a lecturer or enrolling students.

**Operation failed:** Check the entered IDs and required information. Contact the school technician if the problem continues.
