# Exercise 04 — Testing, Invariants, and Sanitizers

## Why this matters
An order book with a subtle matching bug is worse than one that crashes — it silently produces wrong trades. This exercise is about building a safety net around Exercise 03's `OrderBook` before any performance work touches it, so later changes (Phase 5/6) have something to prove they didn't break.

## Decision to make first
Pick a unit test framework — Catch2 or GoogleTest, pulled in via CMake's `FetchContent` (this was deferred back at project setup; now's the time). Either is fine and industry-standard; Catch2 tends to read a little lighter for this scale of project.

## Requirements

1. Wire up the chosen framework via `FetchContent` in a way that doesn't pollute the exercise executables — tests get their own target.
2. Unit tests against Exercise 03's `OrderBook` covering: resting adds, full matches, partial matches across multiple levels, cancels, and edge cases (cancel of a non-existent id, add with zero quantity, etc. — decide what "invalid" means for your book and test it).
3. Invariant/property checks, run over many randomly-generated order sequences: the book is never crossed (best bid never ≥ best ask after any operation), total quantity is conserved across adds/cancels/fills, no id appears twice as a live resting order.
4. Build and run under the `sanitize` preset (already scaffolded in `CMakePresets.json` from project setup) — fix anything ASan/UBSan flags.
5. Run `clang-tidy` over the order book source and address at least the findings that indicate real bugs (not just style).

## Constraints
- Tests must exercise the real `OrderBook` from Exercise 03 — no mocking the book itself.
- Random/property tests need a fixed seed for reproducibility when something fails.

## Deliverable
- Test suite, buildable and runnable independently of the exercise's driver executable.
- `WRITEUP.md`:
  1. What's the worst bug the random property tests caught that the hand-written unit tests didn't?
  2. Did the sanitizer builds find anything the plain debug build didn't?
  3. Pick one clang-tidy finding you fixed and explain what it was actually protecting against.

## Build & run
```
cmake --build --preset debug
ctest --test-dir build/debug
cmake --build --preset sanitize
ctest --test-dir build/sanitize
```

## Done when
All tests pass under both `debug` and `sanitize` presets, and the property tests have run enough random iterations that you trust the book's invariants.
