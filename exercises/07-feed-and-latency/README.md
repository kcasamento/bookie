# Exercise 07 — Feed Simulation and Latency Harness

## Why this matters
This is the first exercise where the pieces actually run together as a system: a feed producer, the lock-free queue from Exercise 06, the matching engine from Exercise 03/05, and a measurement of how long a message actually takes to travel through all of it — the "tick-to-trade" latency real trading systems are obsessed with.

## Requirements

1. A feed producer thread that generates a synthetic stream of orders (randomized prices/quantities/sides at some configurable rate) and pushes them through the Exercise 06 queue.
2. A consumer thread that pops from the queue and feeds each order into the Exercise 03 `OrderBook`'s `addOrder`/matching path.
3. Timestamp capture at, at minimum: the moment the producer creates the order, and the moment the consumer finishes processing it. Use `std::chrono::steady_clock` (or a higher-resolution clock if your platform offers one).
4. A latency-percentile report — p50, p99, p99.9 — over a real run of at least tens of thousands of orders. Don't just report an average; averages hide exactly the tail latency this kind of system cares about.
5. A minimal low-latency logger: a lock-free ring buffer (yes, the same primitive family as Exercise 02/06 again) of pre-formatted log entries, with a separate background thread draining it to actual output — so nothing in the hot path calls `iostream` directly.

## Constraints
- The hot path (producer → queue → matcher) must not use the logger synchronously — logging is fire-and-forget from the hot path's perspective.
- Timestamps must not become the bottleneck you're measuring — be deliberate about how often you sample the clock.

## Deliverable
- Producer/consumer/logger driver, runnable end-to-end.
- `WRITEUP.md`:
  1. What's your measured p50/p99/p99.9 tick-to-trade latency, and what's the largest single contributor you could identify?
  2. Did adding the logger change your latency numbers at all? If so, why, given it's supposed to be off the hot path?
  3. What would you need to add to this harness to trust these numbers in a real production readiness review?

## Build & run
```
cmake --build --preset release
./build/release/exercises/07-feed-and-latency/ex07_feed_and_latency
```
Run under `release`, not `debug` — latency numbers from a debug build aren't meaningful.

## Done when
You have a real percentile report from a real run, and can point to what's actually driving the tail latency rather than guessing.
