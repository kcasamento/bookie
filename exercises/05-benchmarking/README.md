# Exercise 05 — Benchmarking and Profiling

## Why this matters
This is where Exercise 03's correctness-first `OrderBook` gets measured, and where Exercise 02's pool/ring-buffer primitives finally earn their place — swapped in only where measurement shows they matter, backed by Exercise 04's tests proving nothing broke.

## Requirements

1. Pull in Google Benchmark via `FetchContent`.
2. Benchmark `addOrder` (resting, no cross), `matchOrder` (single fill, multi-level fill), and `cancelOrder` against the Exercise 03 `OrderBook`, at a few different book depths (e.g. 10, 1,000, 100,000 resting orders) — establish real nanosecond-level baselines, not guesses.
3. Profile a benchmark run (Instruments on macOS, or `perf` on Linux) and identify the actual hottest functions — compare against what you assumed would be hot before profiling.
4. Based on what profiling shows, replace the highest-impact allocation source in the hot path with the Exercise 02 `OrderPool` (or a similar pool), re-run the Exercise 04 test suite to confirm correctness held, then re-run the benchmarks and record the delta.
5. Confirm zero heap allocations remain in the specific hot path you optimized — verify this concretely (an allocation-counting custom allocator, or a sanitizer/tooling option that reports allocation counts), not just by inspection.

## Constraints
- Don't optimize before profiling. If intuition about "the hot part" turns out wrong once you actually measure, that's the point of the exercise, not a failure.
- Every optimization must be justified by a before/after benchmark number in the writeup — no unmeasured changes.

## Deliverable
- Benchmark suite, runnable independently.
- `WRITEUP.md`:
  1. What did you expect to be the hottest function before profiling, and what actually was?
  2. Before/after numbers for whichever optimization you made, plus the allocation-count proof.
  3. What's still slow that you're deliberately leaving for Phase 6 (concurrency) or later?

## Build & run
```
cmake --build --preset release
./build/release/exercises/05-benchmarking/ex05_benchmarking
```
Benchmarks should be run under the `release` preset — debug-build numbers are not representative.

## Done when
You have real before/after numbers for at least one profiling-driven optimization, and can explain in the writeup exactly why the profiler pointed where it did.
