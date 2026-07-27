# Exercise 06 — Concurrency: Lock-Free SPSC Queue and Snapshot Publishing

## Why this matters
Exercise 02's `RingBuffer` was deliberately single-threaded — this is where it gets formalized into something a producer thread and a consumer thread can actually share safely, plus a mechanism for a separate "risk" reader thread to see consistent book snapshots without ever blocking the matching thread. This is also where the cache-line/false-sharing scope note from Exercise 02 finally applies for real.

## Requirements

1. Take the Exercise 02 `RingBuffer` and make it genuinely safe for one producer thread and one consumer thread operating concurrently: replace plain indices with `std::atomic`, and reason explicitly about which operations need `memory_order_acquire`/`memory_order_release` (hint: the producer's write must be visible to the consumer *before* the consumer sees the updated index, and vice versa for reclaiming space).
2. Separate the producer's and consumer's index/cache-line-sensitive state with `alignas(64)` padding, and demonstrate the false-sharing effect this avoids — benchmark the padded vs. unpadded version under real concurrent load and show the difference.
3. Build a mechanism (a seqlock or double-buffer) for a reader thread to obtain a consistent snapshot of the order book's top-of-book state, published by the matching thread, without the matching thread ever blocking on the reader.
4. Stress-test under `ThreadSanitizer` (a new preset you'll need to add, mirroring the existing `sanitize` preset but with `-fsanitize=thread` instead) — run enough concurrent iterations to have confidence there's no data race.

## Constraints
- No mutexes on the hot path — the entire point is lock-free single-producer/single-consumer and non-blocking snapshot publishing.
- Get the memory-ordering reasoning right conceptually before reaching for the loosest ordering that "seems to work" — a race that doesn't reproduce isn't a race that doesn't exist.

## Deliverable
- Updated `RingBuffer`, the snapshot-publishing mechanism, and a driver running real producer/consumer/reader threads under load.
- `WRITEUP.md`:
  1. For each atomic operation in your queue, what memory order did you use and why — what specifically would break with a looser one?
  2. What was the measured false-sharing cost (padded vs. unpadded), and does it match your expectation?
  3. How does the snapshot mechanism guarantee the reader never sees a torn/inconsistent update?

## Build & run
```
cmake --build --preset debug
./build/debug/exercises/06-concurrency/ex06_concurrency
```
Add and use a `tsan` configure preset for the ThreadSanitizer runs.

## Done when
The stress test runs clean under ThreadSanitizer, and the false-sharing benchmark shows a measurable, explained difference.
