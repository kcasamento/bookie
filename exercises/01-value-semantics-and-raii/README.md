# Exercise 01 — Value Semantics, Copies, and RAII

## Why this matters
In an order book, an `Order` (or a price level's queue of orders) gets copied, moved, stored, and destroyed constantly on the hot path. Every unintended copy is wasted work you can't get back in a system optimizing for nanoseconds. Before building anything book-related, you need to be able to *see* — not just reason about — when C++ copies, moves, or constructs your types.

## Objective
Build an instrumented type and a small driver program that make copy/move/construction/destruction behavior visible, then explain what you observe.

## Requirements

1. Define a struct/class `Order` with at least: `id` (integral), `price` (integral — treat as ticks, not floating point), `qty` (integral), `side` (an enum: `Buy`/`Sell`).
2. Instrument *every* special member function (default ctor, parameterized ctor, copy ctor, move ctor, copy assignment, move assignment, destructor) to print which one ran and which `id` it operated on.
3. Write a `main()` that:
   - Constructs a handful of `Order`s directly into a `std::vector<Order>` two ways: once letting the vector grow organically via repeated `push_back`, once after calling `reserve()` up front. Compare the copy/move counts between the two runs.
   - Writes three call paths for a free function that takes an `Order` — by value, by `const&`, and by `Order&&` (three overloads, or three distinctly named functions). Call each from `main` and observe what triggers.
   - Stores `Order`s in a `std::map<int, Order>` keyed by `id` and observe what happens on insertion via `operator[]`/`insert` vs. `emplace`.
4. Build a separate `ScopedTimer` RAII type (`std::chrono::steady_clock`-based) that prints elapsed time when it goes out of scope. Use it to time the vector-fill loop in both variants from step 3.

## Constraints
- No heap allocations beyond what `std::vector`/`std::map` do internally — don't hand-roll containers yet, that's a later phase.
- `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion` are on for this whole project (see top-level `CMakeLists.txt`) and this exercise should build clean, warning-free.

## Deliverable
- `main.cpp` implementing the above.
- A short `WRITEUP.md` in this same directory answering:
  1. How many copies/moves did `reserve()` save you, and why?
  2. Which of the three call styles (`by value` / `const&` / `&&`) triggered a copy, and under what call-site conditions did the `&&` overload actually get picked?
  3. Did `map::emplace` avoid a construction that `operator[]`/`insert` didn't? Why?

## Build & run
```
cmake --preset debug
cmake --build --preset debug
./build/debug/exercises/01-value-semantics-and-raii/ex01_value_semantics
```

## Done when
- Build is warning-free.
- You can explain, in your own words in `WRITEUP.md`, every copy/move that printed — not just that it happened, but why the compiler chose that path.
