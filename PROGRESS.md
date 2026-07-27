# Progress Notes

Last updated: 2026-07-27 — session paused here. Read this before resuming, whether that's the same person or a different agent picking this back up.

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
Status: **core implementation complete and reviewed clean — driver (`main.cpp`) and `WRITEUP.md` still outstanding**

- `src/order.hpp`, `src/trade.hpp`, `src/price_level.hpp`/`.cpp`, `src/order_book.hpp`/`.cpp` are all written, wired into the `orderbook` library target (`src/CMakeLists.txt`), and compile warning-free under the project's full flag set.
- Went through several real rounds of review with genuine bugs found and fixed along the way (inverted crossing conditions, a dangling-reference-after-`pop_front` caught with ASan, `std::remove_if` used without `erase`, `matchOrder` discarding its own return value, resting logic unreachable whenever the opposite side was empty, an id→location index that was never populated, and a refactor that briefly caused every order to rest into `bids_` regardless of side) — all confirmed fixed via compiled scratch tests, not just re-reading the diff.
- Verified end-to-end against all five required driver scenarios (rests without crossing, full match, partial match across multiple resting orders/price levels, cancel on both sides, best bid/ask updating correctly throughout) — all pass, clean build.
- Still needed per the README: the actual `exercises/03-order-book-core/main.cpp` driver printing these scenarios, and `WRITEUP.md` (price-time-priority guarantee, partial-fill walkthrough, Big-O of `addOrder`/`cancelOrder`). **Likely to be satisfied by Exercise 04's test suite instead of a separate hand-written driver** — see below; confirm with the user which they want before assuming this is skippable.

### Exercise 04 — Testing, Invariants, and Sanitizers
Status: **hand-written unit tests + AI-assisted property test done; a real, unresolved use-after-free bug found in `OrderBook::cancelOrder` — this needs a decision/fix before moving on; clang-tidy pass and `WRITEUP.md` still outstanding**

- `exercises/04-testing-and-hygiene/CMakeLists.txt` pulls Catch2 v3.7.1 via `FetchContent`, builds `ex04_orderbook_tests` linked against `orderbook` + `Catch2::Catch2WithMain`, and registers each `TEST_CASE` with `ctest` via `catch_discover_tests`. Verified end-to-end: fetch, build, and `ctest --test-dir build/debug` all work (had to add `set_target_properties(Catch2 Catch2WithMain PROPERTIES COMPILE_OPTIONS "")` after `FetchContent_MakeAvailable` — the top-level `add_compile_options` otherwise leaks into Catch2's own source and produces warning noise unrelated to this project's code).
- Top-level `CMakeLists.txt` now has `include(CTest)` added, enabling `ctest` project-wide.
- `example_syntax_test.cpp` (scaffolding) has been superseded — real tests now exist.
- `order_book_test.cpp`: **hand-written by the user**, four `TEST_CASE`s (rests without crossing, full match, full match across multiple resting orders/levels, cancel of a resting order). All pass under `debug`.
- `order_book_property_test.cpp`: **written with AI assistance, not hand-written** — an explicit, one-time exception to this curriculum's normal "never write the user's test files" rule, made at the user's request because they wanted to move on to the next order-book area rather than spend more time on testing mechanics they'd already understood conceptually. Uses `GENERATE` over 5 fixed seeds, a `std::mt19937` per seed driving 200 random add/cancel operations (price range 950–1050 shared across both sides so crossing actually happens), and checks two invariants after every operation: book never crossed (`bestBid() < bestAsk()`), and no order is ever matched for more than its remaining quantity (tracked via a shadow ledger keyed off the `Trade::buy_side_id`/`sell_side_id` the book itself returns, not by reimplementing matching).
- **This property test immediately (seed=1, step 9) found a real, 100%-reproducible bug, confirmed two ways:** a plain `debug`-preset run crashes with `SIGSEGV`; the `sanitize`-preset run (ASan) reports a precise **heap-use-after-free** in `PriceLevel::cancelOrder`, called from `OrderBook::cancelOrder`, on memory freed earlier inside `OrderBook::matchOrder`.
  - Root cause: `OrderBook::orders_` (`order_book.hpp:22`) stores a raw `PriceLevel*` per order id, pointing into `bids_`/`asks_` (`std::map<size_t, PriceLevel, ...>`). When `OrderBook::matchOrder` (`order_book.cpp:51` and `:67`) erases a price level from `bids_`/`asks_` because it became empty, nothing removes or updates the `orders_` entries that still point at that now-destroyed `PriceLevel`. A later `cancelOrder(id)` for any id that was ever recorded under that erased level dereferences freed memory.
  - Not yet fixed — this is the user's production code (Exercise 03's `OrderBook`), not a test file, so per this curriculum's review convention it wasn't patched without being asked. Needs a decision from the user on how to fix `orders_`'s staleness (e.g., purge/update the relevant `orders_` entries whenever a `PriceLevel` is erased or an order is matched away).
- Sanitizer preset (`cmake --preset sanitize`) is now configured and working (`build/sanitize`) — the initial `FetchContent` git clone step failed under this session's sandbox (couldn't copy git hook templates); had to disable the sandbox for that one configure step. Not itself a code issue.
- clang-tidy pass over `src/*.cpp`: not yet run.
- `WRITEUP.md`: not written — explicitly left for the user (not part of the AI-assistance exception above). The use-after-free above is a strong, real candidate answer for the WRITEUP's "worst bug the property tests caught" question.

### Exercises 05–08
Not yet started, but fully specified and scaffolded — each has a `README.md` spec, a `CMakeLists.txt`, and a placeholder `main.cpp` (empty stub, builds cleanly, no logic), exactly like exercises 01/02 were set up. **Not yet rewritten to the lesson-first README convention** (see `ROADMAP.md`'s README-convention note) — do that when actually reached, informed by real problems encountered, not speculatively ahead of time. See `ROADMAP.md` for the phase-to-exercise mapping and a one-line summary of each.

## How the user likes to work (read this before jumping in)

- **Never write or edit their exercise `main.cpp`/`WRITEUP.md` files for them.** They want to write every line of implementation themselves. Guidance, worked *analogous* examples (different type names — `Widget`, `Gadget`, `Holder`, never their actual `Order`/`OrderPool`), and compiled scratch demos (written to the session scratchpad dir, not the repo) are all fine and have been the effective pattern throughout. When reviewing their code for bugs, it's fine to point at exact lines/behavior and even compile a copy in scratch to prove a bug is real — just don't patch their file.
- Is rebuilding low-level C++ intuition after a long time away. Comfortable with general OOP concepts, but wants fundamentals (references vs. pointers, value categories/lvalue-rvalue, templates, forwarding references) re-derived carefully from first principles, not asserted — and specifically likes seeing claims *proven* with real compiled output rather than taking them on faith. Lean on scratch compilation heavily; it's been the highest-value teaching tool in this session, more so than prose explanation alone.
- Wants to move at a reasonable clip toward the actual order book (Phase 3) — explicitly comfortable leaving exercises 01/02 partially finished and returning to them later, rather than gating all forward progress on 100% completion first. Don't insist on finishing 01/02 before letting them move on if they want to press forward.
- Appreciates being told directly when something they wrote is a real, confirmed bug (with evidence) vs. just a style preference — has asked for compiled proof multiple times before accepting an explanation.

## Environment notes

- The toolchain is AppleClang (via Xcode), not real GNU GCC, despite `/usr/bin/gcc` existing — that's a Clang shim on macOS. This is expected and does not need fixing.
- Earlier in this session, a stale `CPLUS_INCLUDE_PATH` environment variable (pointing at a mismatched/stale CommandLineTools SDK path) broke header resolution for `<chrono>` and caused a confusing `intmax_t`/`using_if_exists` compile error, unrelated to any actual code. It was resolved by the user restarting their tmux session. If a similar SDK/header error reappears, check `echo $CPLUS_INCLUDE_PATH` first before assuming it's a real code bug.
