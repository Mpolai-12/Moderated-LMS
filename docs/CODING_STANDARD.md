# Coding standard (Moderated LMS, CS4433)

Language: C++17, no compiler extensions. Built with CMake. Warnings are errors
(`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`).

## Naming
| Thing | Style | Example |
|---|---|---|
| Types (class, struct, enum) | PascalCase | `SubmissionService` |
| Functions and methods | camelCase | `effectiveDeadline()` |
| Variables, parameters | camelCase | `daysLate` |
| Private data members | camelCase with trailing underscore | `strategy_` |
| Constants | `kPascalCase` | `kDeadline` |
| Files | one class per file, same name as the class | `Submission.hpp` / `.cpp` |
| Namespace | `lms` | |

## Layout rules
1. Four spaces, no tabs. Lines of about 110 characters or fewer.
2. Header guard: `#pragma once`. Include order: own header, standard library, third-party, project headers.
3. Headers hold declarations; logic goes in `.cpp` files (small inline getters are fine).
4. Braces on the same line as the statement. Always use braces for multi-line bodies.

## Design rules (these protect the architecture)
1. **Layers.** `ui -> services -> domain`; `data` implements domain interfaces. The domain layer includes nothing from `data`, `services`, `ui` or SQLite.
2. **No SQL outside `src/data`.** Every query is parameterised (`sqlite3_bind_*`), never built by joining strings.
3. **No plain-text passwords.** Only the Argon2id hash string is stored or passed around.
4. **Errors.** Throw a type derived from `LmsError` (never a bare string or `int`). Only `main` and the menu loop catch everything; other code lets errors travel up.
5. **Ownership.** `std::unique_ptr` for owned objects, references for borrowed ones. No `new`/`delete`, no raw owning pointers.
6. **Dependencies are passed in** through constructors (dependency injection); `main` is the only place that builds concrete classes.
7. **Time comes from `IClock`**, never from `system_clock::now()` inside business logic, so deadlines are testable.
8. Role checks live in services (`AccessControl::require`), not in menus.

## Comments
Explain *why*, not *what*. Every public class has one comment line saying its job.

## Review checklist (used for the self code review)
- [ ] Does the code follow the class and sequence diagrams?
- [ ] Any SQL built from user text? Any plain-text password?
- [ ] Every input validated before use? Every error type sensible?
- [ ] Any business rule in the menus? Any role check missing in a service?
- [ ] Is each new rule covered by a test, including the boundaries?
