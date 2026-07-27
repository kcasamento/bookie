# Exercise 08 — Production Build and Delivery

## Why this matters
Everything so far has lived inside individual exercise executables. This is where the actual order book engine gets extracted into something structured the way a real, shippable C++ project is — a library other things link against, with tests and benchmarks as first-class consumers of it, and CI enforcing that it stays that way.

## Requirements

1. Extract the order book engine (from Exercises 03/05/06/07, whatever's accumulated by this point) out of the exercises tree into a proper `src/` + `include/` library target, e.g. `orderbook_lib`.
2. Add a thin `app/main.cpp` executable that links against `orderbook_lib` — this becomes the actual product entry point, separate from any exercise.
3. Point the Exercise 04 test suite and Exercise 05 benchmark suite at `orderbook_lib` directly, instead of duplicating engine code inside the exercise tree.
4. Set up GitHub Actions CI: build (debug + release), run the test suite, run the sanitizer build, on every push. No merging past a red CI run.
5. Add versioning (even a simple `PROJECT_VERSION` in the top-level `CMakeLists.txt` is enough) and finalize the top-level `README.md` to describe the real product, not just the exercises.

## Constraints
- `exercises/` stays as-is — a learning history, never linked into the real product.
- CI must actually run all of: build, tests, sanitized build. A CI config that only builds is not sufficient.

## Deliverable
- Restructured repo with `src/`, `include/`, `app/`, `tests/`, `benchmarks/`, and `exercises/` all coexisting.
- A working `.github/workflows/ci.yml` (or equivalent) with a passing run visible on the repo.
- `WRITEUP.md`:
  1. What had to change in the test/benchmark targets to point at the library instead of duplicated code?
  2. What did setting up real CI catch that local builds hadn't?

## Build & run
```
cmake --build --preset release
./build/release/app/orderbook
```

## Done when
CI is green on a real push, and the top-level README accurately describes how to build and run the actual product.
