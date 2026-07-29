# Exercise 05 — Benchmarking and Profiling

## Why this matters
Exercise 03 built the book's semantics; Exercise 04 built the safety net that proves those semantics still hold after a change — including catching a real heap-use-after-free in `OrderBook::cancelOrder` that a hand-written test alone hadn't found. This exercise is where that safety net starts earning its keep: you're now allowed to touch the hot path, because you have a fast, automatic way to know if you broke it. This is also where Exercise 02's `RingBuffer`/`OrderPool` — built and verified in isolation, but never plugged into anything real — finally get a chance to prove they're worth using, and only where actual measurement says they matter.

---

## Lesson: why "measure before optimizing" is a rule, not a suggestion
Intuition about what's slow in a program is wrong more often than it feels like it should be — not because you're bad at this, but because modern hardware's actual cost model (cache misses, branch mispredictions, allocator contention) doesn't match the mental model most people reach for by default ("more lines of code = slower", "a loop = the expensive part"). Optimizing without measuring first risks two failure modes at once: you spend real effort speeding up something that was never the bottleneck, and you have no number to point to afterward proving you actually improved anything. Benchmarking isn't a formality before the "real" optimization work — it's what tells you where the real work is.

## Lesson: what Google Benchmark actually measures, and how to not lie to yourself with it
A benchmark function gets called repeatedly; the timed region is a loop over `state`:
```cpp
static void BM_Foo(benchmark::State& state) {
  for (auto _ : state) {
    // only this is timed
  }
}
BENCHMARK(BM_Foo);
```
Google Benchmark calls `BM_Foo` itself multiple times (to calibrate how many iterations produce a stable measurement, and again for repetitions) — code written *before* the `for` loop runs once per call to `BM_Foo`, not once per iteration inside it. That distinction matters a lot for `OrderBook`, because the operations you're benchmarking *mutate* the book:

- If you want to measure "add a resting order to a book that already has 1,000 orders in it," building that 1,000-order book belongs *before* the loop (setup cost, not what you're measuring) — but then the very first timed iteration changes the book to 1,001, the next to 1,002, and so on. By iteration 5,000 you're not measuring the same scenario you started with, and the benchmark's averaged number quietly blends many different book depths together.
- Two ways to keep iterations comparable: use `state.PauseTiming()`/`state.ResumeTiming()` to bracket a per-iteration reset (rebuild the book, or remove what you just added, outside the timed region) — or, for `cancelOrder`, add a fresh order to cancel on every iteration inside a paused region, so what's actually timed is just the cancel. Either way, the goal is the same: the thing that changes between iterations should be reset before it can bias the next one.
- To sweep across book depths (10 / 1,000 / 100,000 resting orders), parameterize instead of copy-pasting: `BENCHMARK(BM_Foo)->Arg(10)->Arg(1000)->Arg(100000);`, and read the value inside via `state.range(0)`.

## Exercise: wire up Google Benchmark and write baseline benchmarks
1. Pull in Google Benchmark via `FetchContent` in `exercises/05-benchmarking/CMakeLists.txt` — same shape as Exercise 04's Catch2 setup (`FetchContent_Declare`/`FetchContent_MakeAvailable`, link the real `orderbook` library, not a reimplementation). Google Benchmark supplies its own `main()` (link `benchmark::benchmark_main`, or call `BENCHMARK_MAIN()` yourself) the same way Catch2's `Catch2WithMain` did.
2. Benchmark `addOrder` (resting, no cross), `matchOrder` (single fill, and a fill that spans multiple resting orders/price levels), and `cancelOrder`, each at a few book depths (10, 1,000, 100,000). Get real nanosecond-level numbers — not guesses — before touching anything.

## Lesson: sampling profilers, self time vs. total time, and a real build-flag gotcha
Instruments (macOS) and `perf` (Linux) are *sampling* profilers: they interrupt the program at regular intervals and record the call stack at that instant, then build a percentage breakdown out of thousands of samples. Two things worth knowing before you read one:
- **Self time vs. total time.** Total time for a function includes everything it calls; self time is only the work done directly inside it. A function can dominate "total time" purely by calling something expensive, without itself being anywhere near the actual bottleneck — always check which one you're looking at before concluding "this function is slow."
- **Frame pointers.** A sampling profiler reconstructs the call stack by walking frame pointers. Compilers routinely omit them at `-O2`/`-O3` by default (it frees up a register) — this project's `sanitize` preset explicitly re-enables them (`-fno-omit-frame-pointer`, see `CMakePresets.json`), but the `release` preset (which is what you're supposed to benchmark under) currently does not. If a profiler's output on a release build looks like it's missing frames or attributing time to nonsense call paths, this is very likely why. Adding `-fno-omit-frame-pointer` to get a trustworthy profile costs a negligible amount of performance — the timing numbers stay representative, only the profiler's ability to see the stack changes.

## Exercise: profile a benchmark run and find the actual hottest function
Profile one of your benchmarks (Instruments' Time Profiler template, or `perf record`/`perf report`) and identify what's actually hot — by self time, not total time. Write down what you expected before you looked. Don't be surprised if they don't match; that mismatch is the point of this step, not a sign you did something wrong.

## Lesson: why Exercise 02's `OrderPool` can't just be dropped in
`OrderPool<T, Capacity>` as it exists right now lives inside `exercises/02-fixed-capacity-structures/main.cpp`, as a template written against that exercise's own throwaway local `Order`/`Side` — not the real ones in `src/order.hpp` that `OrderBook`/`PriceLevel` actually use. Nothing in `src/` can `#include` it yet. Before it's usable here, it needs the same promotion Exercise 03 gave `Order`/`Trade`: out of an exercise-local sandbox and into a real shared header (e.g. `src/order_pool.hpp`) that both the `orderbook` library and this exercise's benchmarks can include.

Once it's promoted, there's a second question profiling itself will answer, not something to assume up front: *which* allocation is actually the one worth replacing. `PriceLevel::orders_` is a `std::deque<Order>` — a pool sized for `Order` objects can plausibly stand in for that. But `OrderBook::bids_`/`asks_`/`orders_` are all `std::map`s, whose allocations are tree *nodes*, not bare `Order`/`PriceLevel` objects — a pool built for `T` objects doesn't directly substitute for `std::map`'s own internal node allocation without an actual allocator adapter. Don't decide which of these to attack before profiling tells you which one is actually costing you time; it may not be the one that seems most obvious from just reading the code.

## Exercise: swap in the highest-impact allocator, re-verify, re-measure
1. Based on what profiling actually showed (not what you assumed before profiling), replace that allocation source with the promoted `OrderPool` (or a similar pool/allocator, if the hot spot turns out to be a `std::map` node rather than a plain `Order`).
2. Re-run Exercise 04's full test suite (`ctest --test-dir build/debug`, then `build/sanitize`) before trusting the change at all — a faster book that's now wrong isn't a win. The suite exists precisely so a change like this can be verified quickly instead of by hand.
3. Re-run the benchmarks and record the before/after delta.

## Lesson: proving zero allocations, not assuming it from reading the diff
"I replaced `new` with a pool call" is not the same as "this path allocates nothing" — a copy, a `std::vector` growth somewhere else in the call chain, or a container you didn't expect to touch memory can all still be allocating. Two ways to get an actual number instead of a guess:
- Override the global `operator new`/`operator delete` (or use a small counting allocator type) to increment/decrement a counter, and check it's unchanged immediately before/after the specific call under test. Keep the measured window as tight as possible around just that call — a whole-program count picks up allocations from Benchmark's or the C++ runtime's own internals and will mislead you.
- Or sidestep modifying code entirely: Instruments' Allocations template (macOS) or `heaptrack` (Linux) will show you allocation counts for a run without any source changes.

## Constraints
- Don't optimize before profiling. If intuition about "the hot part" turns out wrong once you actually measure, that's the point of the exercise, not a failure.
- Every optimization must be justified by a before/after benchmark number in the writeup — no unmeasured changes.
- Whatever you change, `OrderBook`'s externally observable behavior (trades produced, `bestBid()`/`bestAsk()`, cancel semantics) must stay identical — Exercise 04's suite is what proves that, not a re-read of the diff.

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
