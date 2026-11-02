# Static Analysis Report

## 1. Tool

Cppcheck 2.21.0 was used to perform static analysis on the Moderated LMS project.

## 2. Scope

The analysis covered:
- `src`
- `tests`

The following checks were enabled:
- Warning
- Style
- Performance
- Portability

The project was analyzed using the C++17 standard.

## 3. Results

Cppcheck successfully analyzed 15 source and test files.

Three distinct style findings were reported.

### Finding 1: Redundant condition

**Location:** `src/app/main.cpp:242`

Cppcheck identified that `s.colInt(6) == 1` is always true at the point where `fields.active` is assigned, because the enclosing condition already checks that value.

**Assessment:** Reviewed. No change made during this QA review.

### Finding 2: Unused struct member

**Location:** `src/app/main.cpp:204`

Cppcheck reported that `Session::username` is not used.

**Assessment:** Reviewed. No change made during this QA review.

### Finding 3: Function could be static

**Location:** `src/domain/MarkCalculator.cpp:8`

Cppcheck suggested that `MarkCalculator::daysLate()` could be declared static because it does not use object state.

**Assessment:** Reviewed. No change made during this QA review.

## 4. Conclusion

Cppcheck completed the analysis without reporting errors.

The three style findings were reviewed and documented. No application code was modified solely to eliminate style warnings.

The complete diagnostic output is available in `cppcheck-report.txt`.
