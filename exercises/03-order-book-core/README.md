# Exercise 03 — Single-Threaded Order Book Core

## Why this matters
This is the exercise where everything so far starts paying rent — but deliberately not all at once. Build the book's *semantics* first using ordinary library containers (`std::map`, a queue), get price-time priority and matching genuinely correct, and only then does Phase 5 come back to replace the slow parts with the pool/ring-buffer primitives from Exercise 02. Optimizing a data structure whose behavior you haven't verified yet is a common real-world mistake — this exercise is structured to avoid it.

This is also the first exercise whose code isn't a throwaway single-file sandbox — what you write here becomes the actual, permanent order book, split across real header/source files with a real library target, the way it'd look in a production repo. The lessons below cover that project structure alongside the order-book domain concepts, in the order you'll actually need them.

---

## Lesson: what a limit order book actually does
A limit order book has two sides — bids (buy orders) and asks (sell orders) — each organized by price level. Within a price level, orders are matched in arrival order (FIFO) — this is "price-time priority": better prices match first, and among equal prices, earlier orders match first. An incoming order that crosses the best price on the opposite side generates a trade (a "fill"), potentially partially filling either side, and continues matching against subsequent price levels/orders until it's either fully filled or no longer crosses.

**One scoping note, worth stating explicitly:** one `OrderBook` instance models exactly *one* instrument/symbol. Routing orders to the right book when you have many symbols is a real concern in production, but it's a higher-level component's job (something that would hold, say, a `std::unordered_map<std::string, OrderBook>` and dispatch by symbol) — it's out of scope here on purpose, so `OrderBook` itself stays symbol-agnostic and focused purely on matching mechanics.

## Lesson: project structure — headers, a real library, and linking
Everything from here on gets split the way a production C++ project actually is: a `.hpp` declares a type's shape (member variables, method *signatures* only), a matching `.cpp` defines the method *bodies*, and `src/CMakeLists.txt` bundles those `.cpp` files into one library target (`add_library(orderbook STATIC ...)`) that the exercise's own executable links against (`target_link_libraries(ex03_order_book_core PRIVATE orderbook)`). `main.cpp` becomes a pure driver — it calls into the library, it doesn't contain any book logic itself.

A few conventions that matter here:
- `#pragma once` at the top of every header — prevents the same header's text being pasted twice into one translation unit.
- Never `using namespace ...;` inside a header — it leaks into every file that includes it, however indirectly.
- A method defined *outside* its class body in a header (rather than inline inside `{ }`) must be marked `inline`, or you'll get a linker "duplicate symbol" error the moment two `.cpp` files include that header — this is why `Order`/`Trade`'s small methods just live inside the class body instead.
- Mark any method that doesn't mutate its object `const` (e.g. `bestBid()`, `empty()`) — this is what lets callers hold your types by `const&` and still query them.
- When you add a new `.cpp` file (e.g. `price_level.cpp`), it needs to be added to `add_library(...)`'s source list in `src/CMakeLists.txt`, or its symbols never get compiled into the library and you'll get an "undefined symbol" error at *link* time, not compile time.

## Exercise: Part 1 — `Order` (`src/order.hpp`)
Port `Order` over from Exercise 01/02, minus anything that was there purely to demonstrate copy/move behavior for its own sake. It needs: `id`, `price`, `quantity`, `side` (an enum: `Buy`/`Sell`).

**Decision to make deliberately, not by default:** signed or unsigned for `id`/`price`/`quantity`? Unsigned (`size_t`) encodes "this can never be negative" as a type-level invariant. The cost: any subtraction that produces a logically-negative intermediate result (e.g. a bug that overfills a resting order) silently wraps to a huge number instead of visibly going negative — `assert(x >= 0)` becomes permanently true and useless for an unsigned `x`. Whichever you pick, `Trade`'s fields (next) need to match it exactly, or every place they interact triggers `-Wsign-conversion` warnings under this project's build flags.

## Exercise: Part 2 — `Trade` (`src/trade.hpp`)
A `Trade` is a record of something that already happened — `buy_side_id`, `sell_side_id`, `trade_price`, `size`. It's never mutated after construction, so its fields can be `const`, and it doesn't need most of the special-member-function ceremony `Order` got.

**Lesson: don't write a destructor just because `Order` had one.** If `Trade` owns no resource (it doesn't — just plain integers), a hand-written destructor — even a totally empty `~Trade() {}` — actively costs you something: it makes the type stop being "trivially copyable" (`std::is_trivially_copyable_v` flips to `false`), even though nothing about its actual behavior changed. This is Exercise 01's Rule of Zero lesson again: if the compiler-generated version does the right thing, don't write your own. Skip the destructor entirely.

## Lesson: the data structures underneath `OrderBook`
```
OrderBook
├── bids_: std::map<price, PriceLevel>   (comparator/access: highest price = "best")
└── asks_: std::map<price, PriceLevel>   (default order: lowest price = "best")

PriceLevel
└── orders_: std::deque<Order>            (FIFO — oldest at front)
```
`std::map` is a sorted tree, not a dense array — it only ever holds entries for prices that currently have at least one resting order. A book with resting bids at only 99 and 100 has exactly two entries in `bids_`, not one for every price from 0 to 100. Erase a price level's entry once it empties out; insert a new one only when an order needs to rest at a price with no existing entry. This sparseness is *why* `n` in this exercise's Big-O questions means "number of distinct occupied price levels" — not total order count, and not price range.

Two decisions to make before writing `OrderBook`'s header:
1. **How "best" is exposed.** `asks_` wants lowest-first, which is `std::map`'s default — `begin()` just works. `bids_` wants highest-first: either give it a custom comparator (`std::greater<size_t>`) so `begin()` is highest, or leave the default and read `rbegin()` instead. Either is fine; be consistent about which you picked everywhere `bids_`'s best price matters (the crossing check, the match loop, `bestBid()`).
2. **How `cancelOrder(id)` finds the right level.** It only receives an id — no side, no price — so `OrderBook` needs its own index answering "where does this id live right now?" before it can delegate to the right `PriceLevel`. A `std::map<size_t, std::pair<Side, PriceLevel*>>`, kept in sync on every add/fill/cancel, works — and pointers into `bids_`/`asks_` stay valid across inserts/erases of *other* entries, because `std::map` is node-based, not contiguous like `std::vector`. The one invariant you own: never leave a pointer in this index after the `PriceLevel` it points to has been erased from `bids_`/`asks_`.

## Exercise: Part 3 — `PriceLevel` (`src/price_level.hpp` / `.cpp`)
Every order inside one `PriceLevel` shares its exact price (`level_`) — that's the entire reason the type exists. `addOrder`/`matchOrder`/`cancelOrder`/`empty()` all lean on that.

**Lesson: the fill loop.** Matching one incoming order against this level's resting queue is a `while` with two independent stopping conditions:
```cpp
while (o.quantity > 0 && !orders_.empty()) {
  Order& in = orders_.front();
  size_t amt = std::min(o.quantity, in.quantity);
  // ...
}
```
- `o.quantity > 0` — stop once the incoming order is fully satisfied. It may leave the front resting order partially filled, still in the queue.
- `!orders_.empty()` — stop once there's nothing left to fill against. Whatever quantity remains on `o` afterward is what the caller (`OrderBook`) will need to carry to the next price level, or rest.

The incoming order must be a reference (`Order& o`), not pass-by-value — a copy's mutations vanish the moment the function returns, and `OrderBook` needs to see how much of the original order is left after this level.

**Lesson: attribute the `Trade` from the incoming order's side, not a side stored on `PriceLevel`.** Every resting order in this level is structurally guaranteed to be on the *opposite* side of whatever's being matched against it — `OrderBook` only ever calls a bid level's match against a crossing sell, and vice versa. So `PriceLevel` doesn't need to track its own side; `o.side` alone tells you which id is the buyer and which is the seller:
```cpp
if (o.side == Side::Buy) { trades.push_back(Trade(o.id, in.id, level_, amt)); }
else                      { trades.push_back(Trade(in.id, o.id, level_, amt)); }
```
Note it's `level_` as the trade price, not `o.price` or `in.price` — the resting order's price is what a fill executes at (the incoming order gets "price improvement" if it was willing to trade at a worse price than it had to), and every resting order here already shares `level_` by definition.

**Lesson: order of operations around removing a fully-filled resting order.** `Order& in = orders_.front()` is a reference *into* the deque. The instant you `orders_.pop_front()`, `in` is dangling — reading `in.id` afterward is undefined behavior (this is exactly the kind of bug ASan catches as a "container-overflow," and won't necessarily crash every time, which is what makes it dangerous). Capture whatever you need from `in` — or build the `Trade` — *before* popping it, not after.

**Lesson: `std::remove_if` never shrinks a container by itself.** It only reorders elements within the range and returns an iterator to the new logical end — the container's `size()` doesn't change until you actually `erase` that range:
```cpp
orders_.erase(std::remove_if(orders_.begin(), orders_.end(), pred), orders_.end());
```
Always use the **two-iterator** `erase` overload here, not `erase(single_iterator)`. The single-iterator form happens to work if the predicate matches exactly one element, but `remove_if` returns `end()` when *nothing* matches — and `erase(end())` is undefined behavior. Cancelling an id that no longer exists (already filled, already cancelled, a bad id from a caller) is a completely normal case your code needs to survive.

Driver checkpoint before moving on: construct a `PriceLevel`, add a few orders, match a crossing order that only partially drains it, match another that fully drains it, cancel a real id and a nonexistent one, and confirm `empty()` reflects reality at every step.

## Exercise: Part 4 — `OrderBook` (`src/order_book.hpp` / `.cpp`)
With `PriceLevel` working, `OrderBook` is mostly routing:
- **`addOrder(Order)`** — check whether it crosses the opposite side's best price (using whichever "best" access you picked earlier). If it crosses, hand off to `matchOrder`. If not, find-or-create the right `PriceLevel` on the correct side and call its `addOrder`.
- **`matchOrder(Order&)`** — loop across price levels: while the incoming order still has quantity *and* the opposite side's best level still crosses, call that level's `matchOrder`, accumulate the trades it returns, and erase the level from its map if it's now `empty()`. Stop when the incoming order is fully filled or nothing left crosses — any remaining quantity becomes a new resting order via the same path `addOrder` uses.
- **`cancelOrder(id)`** — look up where the id lives via your index, delegate to that `PriceLevel::cancelOrder`, and erase the level if it's now empty.
- **`bestBid()` / `bestAsk()`** — return the best *price* (`std::optional<size_t>`, not a specific `Order` — several orders can rest at the same best price, and there's no single one of them that uniquely represents "the best bid"). Use `std::optional` for the same reason `RingBuffer::pop()` did back in Exercise 02: there's a real "nothing here" case (an empty side) that needs representing, not papered over by undefined behavior from dereferencing an empty map's `begin()`.

## Constraints
- Correctness first. Do not reach for the Exercise 02 primitives here — that swap is Phase 5's job, deliberately deferred so you can verify book *behavior* independent of *performance*.
- Same warning flags as prior exercises; build clean.
- For the trade-list return type (`matchOrder`'s `std::vector<Trade>`), don't reach for a fixed-capacity structure — the number of trades one incoming order generates isn't knowable ahead of running the match, which is exactly the case `std::vector` exists for. `.reserve()`-ing a small guess is a fine, cheap middle ground if you want one, but a real fixed bound isn't available here the way it was for Exercise 02's `RingBuffer`/`OrderPool`.

## Deliverable
- Implementation split across `src/order.hpp`, `src/trade.hpp`, `src/price_level.hpp`/`.cpp`, `src/order_book.hpp`/`.cpp`, wired into a library target in `src/CMakeLists.txt`.
- A driver in `exercises/03-order-book-core/main.cpp` exercising: an add that rests (no cross), an add that fully matches a single resting order, an add that partially fills across multiple resting orders/price levels, a cancel of a resting order, and confirmation that `bestBid()`/`bestAsk()` update correctly after each.
- `WRITEUP.md` answering:
  1. How do you guarantee price-time priority is actually respected — what would break it if you got the container choice wrong?
  2. Walk through exactly what happens (in your implementation) for a partial fill — which side's remaining quantity survives, and where does it end up?
  3. What's the Big-O of `addOrder` and `cancelOrder` in your current design? Where's the cost coming from — and in terms of *what* (occupied price levels? orders at one level? total orders in the book?).

## Build & run
```
cmake --build --preset debug
./build/debug/exercises/03-order-book-core/ex03_order_book_core
```

## Done when
Build is warning-free, the driver demonstrates all five scenarios above with clear printed output, and you can answer the writeup's Big-O question with confidence about *why*, not just what.
