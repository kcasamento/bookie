# Exercise 02 — Fixed-Capacity Structures: Ring Buffer & Memory Pool

## Why this matters
On the hot path you never want to call `malloc`/`free` — allocation latency isn't just slow, it's *unpredictable* (page faults, allocator locks, fragmentation). Two building blocks eliminate that: a fixed-capacity ring buffer for queuing (this becomes your market-data ingestion path in Phase 7, unmodified), and a memory pool/arena that hands out pre-allocated storage for `Order` objects instead of round-tripping through `new`/`delete`. Both come back later as-is — not throwaway.

**Scope note:** this exercise skips cache-line/false-sharing micro-benchmarking on purpose — that only matters once multiple threads are actually contending (Phase 6). Measuring it single-threaded here would just be going through the motions.

## Part A — Fixed-capacity ring buffer

Build `RingBuffer<T, Capacity>` (a class template, `Capacity` a non-type template parameter) backed by a `std::array<T, Capacity>` — zero heap allocation.

Requirements:
1. `push(T value)` — returns `bool`; `false` if full (no throwing, no growing).
2. `pop()` — returns `std::optional<T>` (empty if the buffer is empty).
3. `size()`, `empty()`, `full()`.
4. Track read/write positions with two indices, wrapping via modulo (or a power-of-two capacity + bitmask — your call, justify it in the writeup).
5. Single-threaded only for now — no atomics, no concurrency. That's Phase 6.

Driver in `main.cpp`: push more `Order`s than capacity and confirm `push` correctly reports full; pop everything and confirm FIFO order; confirm `push` succeeds again after popping (proving wraparound actually works, not just a straight-line buffer that happens to pass a smaller test).

## Part B — Fixed-size memory pool for `Order`

Build `OrderPool` that pre-allocates raw storage for `N` `Order` objects up front (e.g. `std::array<std::byte, N * sizeof(Order)>` — raw bytes, not `std::array<Order, N>`, since `Order` isn't default-constructible) and hands out/reclaims individual `Order*` without ever calling `new`/`delete` after the pool's own backing storage exists.

Requirements:
1. `allocate(...)` — placement-new an `Order` into a free slot, forwarding constructor args; returns `Order*` (or `nullptr` if exhausted).
2. `deallocate(Order*)` — explicitly calls the destructor, marks the slot free again.
3. Track free slots however you like — an intrusive free-list threaded through the raw storage is the "real" production approach and worth attempting, but a simple free-index stack is a fine first pass.
4. Driver: allocate more than `N` and confirm graceful failure; deallocate one and confirm the freed slot gets reused (print the pointer to prove it's literally the same address).

## Constraints
- No `new`/`delete`/`malloc` anywhere in either structure after the pool's own backing storage is created.
- Same warning flags apply — build clean.

## Deliverable
- `main.cpp` (single file is fine, or split into headers if you want the practice — your call).
- `WRITEUP.md` answering:
  1. Why modulo vs. bitmask for the ring buffer's index wraparound, and what constraint does the bitmask approach impose on `Capacity`?
  2. What is placement-new actually doing differently from a plain `new Order(...)` call?
  3. What UB would you hit calling `deallocate` twice on the same pointer, and why doesn't the compiler catch it for you?

## Build & run
```
cmake --build --preset debug
./build/debug/exercises/02-fixed-capacity-structures/ex02_fixed_capacity_structures
```

## Done when
Build is warning-free, and you can explain placement-new and the free-list mechanics in your own words in the writeup.
