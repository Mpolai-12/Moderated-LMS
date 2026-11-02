# Moderated LMS — Head of Department User Manual

## 1. Purpose

This manual explains how Heads of Department (HoDs) review marks, moderate lecturer marking decisions, release approved marks to students, and inspect the department audit log.

## 2. Logging in

1. Start the LMS application.
2. Enter your username and password.
3. Your account must be active and have the Head of Department role.
4. Your account must be associated with the correct department.

## 3. HoD menu

The HoD menu provides these options:

1. Review department marks
2. Moderate / release marks
3. View department audit log
0. Log out

Enter the number of the operation you want to perform.

## 4. Review department marks

1. Select **Review department marks**.
2. The system displays submissions in your department that have been marked, moderated, or released.
3. Review the displayed information:
   - Submission ID
   - Course code
   - Assignment title
   - Student name
   - Raw mark and maximum mark
   - Submission state
   - Lecturer feedback

Use this information to identify marks that require moderation.

## 5. Moderate a mark

1. Select **Moderate / release marks**.
2. Review the department marks displayed.
3. Enter the Submission ID you want to moderate.
4. If the submission is in `MARKED` status, the current raw mark and maximum mark are displayed.
5. Choose whether to change the mark by entering `y` or `n`.

If you choose not to change the mark:
- The lecturer's existing raw mark is approved.
- The submission moves to `MODERATED` status.

If you choose to change the mark:
1. Enter the moderated raw mark. It must be between zero and the assignment's maximum mark.
2. Enter the reason for the adjustment. A reason is required.
3. The system records the change in the audit log.
4. The submission moves to `MODERATED` status.

The system confirms that moderation is complete and instructs you to run the action again to release the mark.

## 6. Release an approved mark

Releasing a mark is a separate step after moderation.

1. Select **Moderate / release marks** again.
2. Enter the same Submission ID.
3. When asked whether to release the approved mark to the student, enter `y` to confirm.
4. The system changes the submission status to `RELEASED` and confirms that the mark has been released.

The student can then view the released mark and feedback.

If you do not confirm release, the mark remains moderated and is not yet released.

## 7. View the department audit log

1. Select **View department audit log**.
2. Review the records displayed.

The audit log includes:
- Audit ID
- Submission ID
- Username of the person who changed the mark
- Date and time of the change
- Previous mark
- New mark
- Reason for the change

For a first mark, the previous mark is displayed as `first mark`.

The HoD view is restricted to records associated with the HoD's department.

## 8. Understanding submission states

| State | Meaning |
|---|---|
| `MARKED` | The lecturer has entered a mark and feedback; HoD moderation is required. |
| `MODERATED` | The mark has been approved or adjusted by the HoD; it awaits release. |
| `RELEASED` | The mark has been released and is available to the student. |

## 9. Important responsibilities

- Review marks and feedback carefully before moderation.
- Confirm that the submission belongs to your department.
- Provide a clear reason for every mark adjustment.
- Release marks only after moderation is complete.
- Use the audit log to review recorded marking changes.
- Follow school policies for assessment moderation and student results.

## 10. Common problems

| Problem | What to do |
|---|---|
| Submission is not found | Check the Submission ID. |
| Submission belongs to another department | Only moderate submissions in your own department. |
| Mark adjustment is rejected | Ensure the mark is between zero and the maximum mark. |
| Adjustment reason is rejected | Enter a reason before saving. |
| Submission cannot be moderated | Only `MARKED` submissions can be moderated. |
| Submission cannot be released | Only `MODERATED` submissions can be released. |

## 11. Logging out

Select **0. Log out** when you have finished using the system.
