# Side Quest — Rebuilding OrderBook with Intrusive, Pool-Backed, Array-Indexed Structures

This is a deliberate detour from the main exercise sequence (triggered while working through Exercise 05's benchmarking/profiling), not a numbered phase in `ROADMAP.md`. Goal: take `src/`'s real `Order`/`PriceLevel`/`OrderBook` — not a sandboxed exercise copy — and rebuild its core structures the way a production HFT book actually would, informed directly by what Exercise 05's profiling found (`std::map` node churn on every insert/erase; `std::deque`'s O(n) cancel).

## Ground rules for this side quest
- **Hand-rolled, not `boost::intrusive`** — deliberate choice, made so the mechanics get learned once by building them, not by reading a library's docs.
- **`Order::id` is treated as already being a dense, internally-assigned sequential id.** Real engines usually split "client order id" (arbitrary) from an internal sequential id used for hot-path indexing, resolved via a separate lookup at the edge. That split is explicitly out of scope here — deferred complexity, not an oversight.
- **A bounded price range is assumed.** Real venues trade within a fixed tick grid inside a bounded band (often circuit-breaker-limited) — this side quest takes the same assumption, enabling a fixed-size array instead of a tree for price levels.

## What changes, in order

### Step 1 — Promote `OrderPool` into `src/`, back `Order` storage with it
`exercises/02-fixed-capacity-structures/main.cpp`'s `OrderPool<T, Capacity>` is templated against that exercise's own throwaway local `Order`/`Side` — it needs to move into a shared header (e.g. `src/order_pool.hpp`) and work against the real `Order` in `src/order.hpp`.

A fixed `Capacity` (max resting orders) has to be chosen explicitly — every real matching engine has this constraint; right now it's implicit/unbounded.

`OrderBook` needs to own (or be constructed with) an `OrderPool` instance. From this point on, "an order in the book" means a pointer into pool storage, not a value living inside whatever container currently holds it.

**Why this has to come first:** every later step (the intrusive list, the id array, the price ladder) assumes an `Order` has a stable address for its entire resting lifetime. The pool is what actually guarantees that — without it, the later steps are building on sand.

### Step 2 — Add the intrusive hook to `Order`; replace `PriceLevel::orders_`
`Order` gains a `prev_`/`next_` hook (or a small named hook member, if you'd rather keep it distinct from the payload fields). `PriceLevel::orders_` changes from `std::deque<Order>` to a hand-rolled intrusive doubly-linked list of pool-backed `Order*`s.

- Resting an order (`PriceLevel::addOrder`): push onto the tail — O(1), no allocation (the pool already owns the memory; the list only links).
- Matching (`PriceLevel::matchOrder`): the front of the list is next in time priority — popping it is now an O(1) unlink instead of `deque::pop_front`.
- **Cancelling is the one real interface change, not just a swap.** Today, `PriceLevel::cancelOrder(size_t id)` (`price_level.cpp:43-48`) does its own `std::remove_if` scan to *find* the order by id. That scan should go away entirely — `PriceLevel` should take a direct `Order*` (already resolved by `OrderBook`, see Step 3) and unlink it in true O(1), no search. Worth deciding the exact new signature deliberately rather than accidentally preserving the id-based scan out of habit.

### Step 3 — Rework `OrderBook::orders_` into a flat, pool-backed array
Since `Order::id` is being treated as dense/internal, `orders_` (`order_book.hpp:22`, currently `std::map<size_t, std::pair<Side, PriceLevel*>>`) becomes a flat array (`std::vector<Order*>` or a fixed `std::array`) sized to the pool's `Capacity`, indexed directly by id. A null/sentinel entry means "no live order at this id."

`OrderBook::cancelOrder(id)` becomes: direct array index → `Order*` → feed that pointer into Step 2's new `PriceLevel::cancelOrder(Order*)` → free the pool slot. Note `Side` no longer needs to be stored alongside the pointer in this index — `Order` already carries its own `side` field, so this is a simplification over what's there today, not a capability loss.

### Step 4 — Rebuild `bids_`/`asks_` as a fixed-size price ladder with an incrementally maintained best-bid/ask cursor
Pick a `MIN_PRICE`/`MAX_PRICE`/tick size up front — this is the "bounded price range" assumption made concrete. `bids_`/`asks_` become a fixed array of `PriceLevel` slots, sized `(MAX_PRICE - MIN_PRICE) / TICK_SIZE + 1`, indexed by `(price - MIN_PRICE) / TICK_SIZE`.

`PriceLevel` storage is simpler here than `Order`'s was in Step 1: the ladder array itself *is* the storage (each slot is just "occupied or not"), so no separate pool is needed for `PriceLevel` the way one was needed for `Order`.

`bestBid_()`/`bestAsk_()` stop being `map.begin()` and become a cached index, updated incrementally:
- A level newly becomes the best → direct comparison against the cached best, no scan.
- The current best level empties out → walk the array inward until the next non-empty level or the band boundary (bounded work in practice — consecutive empty levels near the top of book are rare).

**This step is the one most likely to reproduce the exact bug class Exercise 04's property test already caught once** (crossed-book / stale-best invariants) — it's the step where re-running that property test matters most, not just as a final check.

## Suggested checkpoints
Re-run the full Exercise 04 suite (`ctest` under both `debug` and `sanitize` presets) after **each** step individually, not just once at the end. Every one of these four steps touches the same "empty container / stale pointer" danger zone that produced the real heap-use-after-free found earlier — catching a regression one step at a time will be far cheaper than untangling it after all four are done.

## Explicitly out of scope for this side quest
- `boost::intrusive` (deliberately hand-rolled instead)
- Client-id vs. internal-id split (deliberately deferred)
- Concurrency / thread-safety of any of this (that's Phase 6 territory)
