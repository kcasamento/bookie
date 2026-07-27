# Roadmap

Progressive exercises building toward a production-style HFT order book. Each phase gets its own exercise under `exercises/NN-topic/`, each with a `README.md` spec, `CMakeLists.txt`, and a stub `main.cpp`. Solutions are written by hand — no code is provided.

**Picking this back up after a break? Read [PROGRESS.md](PROGRESS.md) first** — it tracks exactly what's done, what's in progress, and any known bugs, per exercise.

**README convention — read before writing or editing any exercise's `README.md`:** each exercise README should teach before it assigns, not just assign. Interleave `## Lesson: ...` sections (the concept, explained, with enough real detail that the exercise doesn't require a live back-and-forth to understand what's being asked or why) directly before the `## Exercise: ...` section that puts that concept into practice — rather than one short "Why this matters" blurb followed by a flat, undifferentiated requirements list. [`exercises/03-order-book-core/README.md`](exercises/03-order-book-core/README.md) is the reference example for this format. Exercises 01/02 predate this convention and haven't been retrofitted; 04-08 should follow it once written/expanded, informed by whatever real problems and lessons actually come up while working through them — don't pre-write speculative lesson content for a phase you haven't lived through yet.

## Phase 0 — Toolchain & repo skeleton
CMake project layout, Debug/Release/Sanitizer build presets, git hygiene. (Done — see top-level `CMakeLists.txt` / `CMakePresets.json`.)

## Phase 1 — Modern C++ mechanics, performance lens → [`exercises/01-value-semantics-and-raii`](exercises/01-value-semantics-and-raii/README.md)
Value semantics, rule of 0/3/5, move semantics, RAII, const-correctness/`noexcept` — applied to a toy `Order` type so copies/moves are observed, not just read about.

## Phase 2 — Memory & data layout → [`exercises/02-fixed-capacity-structures`](exercises/02-fixed-capacity-structures/README.md)
Cache-line effects and false sharing (deferred to Phase 6, where it's actually observable under real contention), a fixed-capacity ring buffer (SPSC queue — becomes the market-data ingestion path later), a custom memory pool/arena for `Order` objects so the hot path never calls `malloc`.

## Phase 3 — Single-threaded order book core → [`exercises/03-order-book-core`](exercises/03-order-book-core/README.md)
`Order`/`Side`/`PriceLevel` modeling, price-level storage (correctness first, with ordinary library containers — the swap to Phase 2's primitives is deliberately deferred to Phase 5), add/cancel, price-time-priority matching, incrementally maintained best-bid/ask.

## Phase 4 — Correctness & production hygiene → [`exercises/04-testing-and-hygiene`](exercises/04-testing-and-hygiene/README.md)
Unit tests against your own book, invariant/property checks (no crossed book, conserved quantity), ASan/UBSan debug builds, clang-tidy.

## Phase 5 — Performance measurement → [`exercises/05-benchmarking`](exercises/05-benchmarking/README.md)
Google Benchmark integrated, nanosecond baselines for add/cancel/match, profiling with `perf`/Instruments, driving hot-path allocations to zero and proving it — this is where Phase 2's pool actually gets wired into the real book.

## Phase 6 — Concurrency → [`exercises/06-concurrency`](exercises/06-concurrency/README.md)
Formalize the SPSC queue with correct `memory_order` semantics, single-writer/multi-reader snapshot publishing (seqlock/double-buffer) so a "risk" thread can read the book without blocking the matcher, `alignas`/padding to kill false sharing (measured for real here).

## Phase 7 — System integration → [`exercises/07-feed-and-latency`](exercises/07-feed-and-latency/README.md)
A simulated feed producer feeding orders through your queue, tick-to-trade latency percentile harness, a low-latency logger (no `iostream` in the hot path).

## Phase 8 — Production build & delivery → [`exercises/08-production-build`](exercises/08-production-build/README.md)
Splitting into a library target + separate app/test/benchmark targets, CI (GitHub Actions) running build+test+sanitizers, README/versioning.

## Capstone
Single-threaded, allocation-free-on-hot-path matching engine with concurrent feed ingestion, full test suite, benchmark suite with recorded baselines, green CI.
