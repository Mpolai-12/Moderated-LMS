# Code Metrics Report

## Tool
Metrics generated using Lizard 1.24.3.

## Scope
The analysis covers:
- `src/domain`
- `src/app`
- `tests`

## Results

| Metric | Result |
|---|---:|
| Files analyzed | 24 |
| Total NLOC | 1,585 |
| Function count | 146 |
| Average function length | 8.6 NLOC |
| Average cyclomatic complexity (CCN) | 3.0 |
| Warning count | 1 |

## Complexity Warning

Lizard reported one function above its default complexity threshold:

- **Function:** `validIsoDate()`
- **Location:** `src/app/main.cpp`, lines 144–153
- **Cyclomatic complexity:** 16

This function checks the structure of UTC ISO timestamps before `askDate()` accepts them. Calendar and time validity are handled separately by the domain-level `fromIso()` function.

The warning is a maintainability consideration, not evidence of a confirmed defect. The function was retained without refactoring to avoid unnecessary changes to working application behavior.

## Raw Results

See [CODE_METRICS.txt](CODE_METRICS.txt) for the complete Lizard output.
