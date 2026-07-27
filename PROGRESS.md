# Progress Notes

Last updated: 2026-07-23 — session paused here. Read this before resuming, whether that's the same person or a different agent picking this back up.

## Where things stand

### Exercise 01 — Value Semantics, Copies, and RAII
Status: **in progress, not complete**

- Done: `Order` fully instrumented across all five special member functions, `ScopedTimer` built and working, the `push_back` (organic growth) vs `reserve` vector comparison implemented and run successfully.
- Still missing, per `exercises/01-value-semantics-and-raii/README.md`:
  - The three pass-by-value / `const&` / `&&` demonstration functions/overloads.
  - The `std::map` `emplace` vs `insert`/`operator[]` comparison.
  - `WRITEUP.md` answering the three writeup questions.
- No known bugs in what's written so far.

### Exercise 02 — Fixed-Capacity Structures: Ring Buffer & Memory Pool
Status: **complete (core implementation) — WRITEUP.md still outstanding**

- Part A (`RingBuffer`): done and verified — push/pop/full/wraparound all behave correctly.
- Part B (`OrderPool`): done. The earlier `std::format`/`std::forward` typo is fixed, `deallocate()` is implemented (destructs, threads the slot onto the free list), and a `main()` driver exercises the full cycle — allocate to exhaustion, deallocate, reallocate, confirm slot reuse. Verified by building `ex02_fixed_capacity_structures` and running it: output shows `o1`/`o2` allocated, `o3` correctly refused at capacity, then after `deallocate(o1)` a new `allocate` reuses that freed slot (dtor for id 1 runs, new order gets id 3 in the reused address). Meets the README's "Done when" criteria.
- `WRITEUP.md` not written yet — left for the user to circle back to per their own pace preference; not blocking forward progress.

### Exercise 03 — Single-Threaded Order Book Core
Status: **assigned, not started** — this is the current exercise.

### Exercises 04–08
Not yet started, but fully specified and scaffolded — each has a `README.md` spec, a `CMakeLists.txt`, and a placeholder `main.cpp` (empty stub, builds cleanly, no logic), exactly like exercises 01/02 were set up. See `ROADMAP.md` for the phase-to-exercise mapping and a one-line summary of each.

## How the user likes to work (read this before jumping in)

- **Never write or edit their exercise `main.cpp`/`WRITEUP.md` files for them.** They want to write every line of implementation themselves. Guidance, worked *analogous* examples (different type names — `Widget`, `Gadget`, `Holder`, never their actual `Order`/`OrderPool`), and compiled scratch demos (written to the session scratchpad dir, not the repo) are all fine and have been the effective pattern throughout. When reviewing their code for bugs, it's fine to point at exact lines/behavior and even compile a copy in scratch to prove a bug is real — just don't patch their file.
- Is rebuilding low-level C++ intuition after a long time away. Comfortable with general OOP concepts, but wants fundamentals (references vs. pointers, value categories/lvalue-rvalue, templates, forwarding references) re-derived carefully from first principles, not asserted — and specifically likes seeing claims *proven* with real compiled output rather than taking them on faith. Lean on scratch compilation heavily; it's been the highest-value teaching tool in this session, more so than prose explanation alone.
- Wants to move at a reasonable clip toward the actual order book (Phase 3) — explicitly comfortable leaving exercises 01/02 partially finished and returning to them later, rather than gating all forward progress on 100% completion first. Don't insist on finishing 01/02 before letting them move on if they want to press forward.
- Appreciates being told directly when something they wrote is a real, confirmed bug (with evidence) vs. just a style preference — has asked for compiled proof multiple times before accepting an explanation.

## Environment notes

- The toolchain is AppleClang (via Xcode), not real GNU GCC, despite `/usr/bin/gcc` existing — that's a Clang shim on macOS. This is expected and does not need fixing.
- Earlier in this session, a stale `CPLUS_INCLUDE_PATH` environment variable (pointing at a mismatched/stale CommandLineTools SDK path) broke header resolution for `<chrono>` and caused a confusing `intmax_t`/`using_if_exists` compile error, unrelated to any actual code. It was resolved by the user restarting their tmux session. If a similar SDK/header error reappears, check `echo $CPLUS_INCLUDE_PATH` first before assuming it's a real code bug.
