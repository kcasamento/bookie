# Exercise 04 — Testing, Invariants, and Sanitizers

## Why this matters
An order book with a subtle matching bug is worse than one that crashes — it silently produces wrong trades. This exercise builds a safety net around Exercise 03's `OrderBook` before any performance work touches it (Phase 5/6), so later changes have something to prove they didn't break. It's also where your "driver" habit changes for good: instead of a `main.cpp` that prints output you eyeball once, you write test cases that assert the right thing themselves, and rerun automatically forever.

**Infrastructure note:** the test framework wiring below is already done for you — `FetchContent`, the CMake target, `ctest` registration, all working and verified. Your job in this exercise is writing the actual test *cases*, not the build plumbing. See `CMakeLists.txt` and `example_syntax_test.cpp` in this directory for what's already there.

---

## Lesson: why a test framework instead of a `main.cpp` driver
Every prior exercise's "driver" was a `main.cpp` that called your code and printed output you had to read and judge yourself — did `trades.size()` look right? Did `bestBid()` update the way it should have? That worked, but it doesn't scale: it requires a human to look at output every single time, it doesn't tell you *which specific* thing broke when something's wrong, and nothing stops you from accidentally deleting a check as the file grows. A test framework fixes all three: each `TEST_CASE` is an independent, isolated check with a pass/fail verdict the framework computes for you (via `REQUIRE`/`CHECK`, not "does this printed number look right"), failures point at the exact assertion and line that failed, and `ctest` can run the entire suite with one command and a single overall exit code — which is exactly what lets Phase 5/6 changes get verified automatically instead of by re-reading console output.

## Lesson: what's already wired up, and how
`exercises/04-testing-and-hygiene/CMakeLists.txt` does four things:
1. `FetchContent_Declare(Catch2 ...)` pulls a specific, pinned version of the Catch2 test framework from its GitHub repo at configure time — pinned by `GIT_TAG` so your build doesn't silently change out from under you when Catch2 releases something new.
2. `add_executable(ex04_orderbook_tests ...)` builds your test file into its own binary, linked against both `orderbook` (your real library — the constraint from Exercise 03 about "no mocking the book itself" holds here) and `Catch2::Catch2WithMain` (a version of Catch2 that supplies its own `main()`, so your test file never needs one).
3. `catch_discover_tests(...)` asks the *built test binary itself* which `TEST_CASE`s it contains, and registers each one as a separate, individually-runnable `ctest` test — this is why `ctest --test-dir build/debug -N` lists each `TEST_CASE` by name, not just one giant "run the tests" blob.
4. The `set_target_properties(... COMPILE_OPTIONS "")` line right after `FetchContent_MakeAvailable` exists because this project's top-level `add_compile_options(-Wall -Wextra ...)` is directory-scoped and would otherwise apply to *Catch2's own source code* too, not just yours — worth knowing this gap exists in `add_compile_options`-based warning setups generally, not just here.

## Exercise: get oriented with `example_syntax_test.cpp`
Build and run the example file as-is first, before writing anything of your own:
```
cmake --build --preset debug --target ex04_orderbook_tests
ctest --test-dir build/debug --output-on-failure
```
Read `example_syntax_test.cpp` alongside the output. Note the three macros in play: `TEST_CASE("name", "[tag]")` declares one independent test; `REQUIRE(...)` stops that test immediately on failure (use it when a later line in the same test would be meaningless or unsafe if this one failed — e.g. dereferencing something that should exist); `CHECK(...)` records a failure but keeps going (use it when you want to see *all* the ways a test failed in one run, not just the first). `SECTION(...)` inside a `TEST_CASE` lets you share setup code across multiple related sub-cases without duplicating it. `GENERATE(...)` re-runs the enclosing `TEST_CASE` once per value you give it — your first taste of the property-style testing below. Once you've seen these actually run and seen a deliberately-broken assertion actually fail (try changing `42` to `43` and rerun), delete this file — it's scaffolding, not something to build on.

## Lesson: example-based tests vs. property/invariant tests
Everything in the example file is *example-based*: you pick specific inputs (21, 0, -5, a handful of generated values) and assert specific expected outputs. That's exactly right for "does a full match produce one trade at the resting price" — concrete scenarios with a known correct answer. But it can't catch a bug that only shows up after some unusual *sequence* of operations you didn't happen to think to write by hand — add, cancel, add again, partially fill, cancel the partial remainder, in some order you didn't script. *Property-based testing* covers that gap differently: instead of asserting a specific output for a specific input, you assert a property that should hold no matter what — "the book is never crossed," "total resting quantity plus total traded quantity equals total quantity ever added," "no id is ever resting twice" — and then throw many random operation sequences at it, checking the property after every single operation. A property test doesn't know what the "right" trade count is for a random sequence; it just knows the book should never end up in an impossible state.

## Exercise: hand-written unit tests against `OrderBook`
Write `TEST_CASE`s (in a new file, e.g. `order_book_test.cpp` — add it to `add_executable`'s source list in `CMakeLists.txt`) covering:
- An add that rests without crossing.
- An add that fully matches a single resting order.
- An add that partially fills across multiple resting orders and multiple price levels.
- A cancel of a resting order — including cancelling an id that doesn't exist (should be a no-op, not a crash — you already fixed exactly this failure mode back in `PriceLevel::cancelOrder`).
- `bestBid()`/`bestAsk()` returning `std::nullopt` on an empty side, and updating correctly after each operation above.
- Zero-quantity orders, and whatever else you decide counts as "invalid" input for your book — write down your reasoning for what you chose to test and why.

## Lesson: writing an invariant/property test by hand (no extra library needed)
You don't need a dedicated property-testing library to get the core benefit — `GENERATE` combined with your own random sequence generation gets you most of the way. The shape: generate a sequence of N random operations (random side, price within some small range so orders actually cross sometimes, random quantity), apply them one at a time to a single `OrderBook`, and after *every* operation, assert your invariants still hold — bid price (if any) is strictly less than ask price (if any), the count of times you've added quantity minus cancelled/traded quantity matches what's still resting, no id appears in more than one resting position. When one of these fails, it'll be on some specific random sequence — print the seed and the operation list so you can reproduce it deterministically, per the constraint below.

## Exercise: property tests + sanitizers + clang-tidy
1. Write the invariant property test described above. Use a fixed seed (don't rely on true randomness — you need to be able to reproduce a failure).
2. Build and run under the `sanitize` preset: `cmake --build --preset sanitize`, `ctest --test-dir build/sanitize`. ASan/UBSan instrument every memory access and undefined-behavior-prone operation at runtime — this is the same tool that caught the dangling-reference-after-`pop_front` and the `erase(end())` bugs during Exercise 03's review, just now running continuously against your test suite instead of a one-off scratch repro.
3. Run `clang-tidy` over `src/*.cpp`. It checks things a compiler warning doesn't — idiom violations, likely-bug patterns (e.g. it would have flagged the `std::remove_if` without `erase` pattern directly), modernization opportunities — rather than just "does this expression's type conversion lose information," which is what your `-W` flags already cover. Fix findings that indicate a real bug; use judgment on pure style suggestions.

## Constraints
- Tests must exercise the real `OrderBook` from Exercise 03 — no mocking the book itself.
- Random/property tests need a fixed seed for reproducibility when something fails.

## Deliverable
- `order_book_test.cpp` (or similar), replacing the deleted example file, registered in `CMakeLists.txt`'s `add_executable` source list.
- `WRITEUP.md`:
  1. What's the worst bug the random property tests caught that the hand-written unit tests didn't?
  2. Did the sanitizer builds find anything the plain debug build didn't?
  3. Pick one clang-tidy finding you fixed and explain what it was actually protecting against.

## Build & run
```
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
cmake --build --preset sanitize
ctest --test-dir build/sanitize --output-on-failure
```

## Done when
All tests pass under both `debug` and `sanitize` presets, and the property tests have run enough random iterations that you trust the book's invariants.
