// Property/invariant test — written with AI assistance rather than fully
// hand-written, per an explicit exception to this curriculum's normal rule.
// See PROGRESS.md for the note on why this file is called out separately
// from order_book_test.cpp (which was hand-written).
#include "order.hpp"
#include "order_book.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <random>
#include <unordered_map>
#include <vector>

namespace {

struct RandomOp {
  bool isCancel;
  size_t id;
  Side side;
  size_t price;
  size_t quantity;
};

// Pulls one random operation's worth of fields from `rng`. Prices are drawn
// from a single narrow, shared range for both sides so buy/sell orders
// frequently cross — a wide/disjoint range would rarely exercise matching
// at all. Cancels target a real previously-used id (which may or may not
// still be resting) most of the time once any ids exist, exercising the
// "cancel something that already fully matched away" path deliberately.
RandomOp nextOp(std::mt19937 &rng, size_t &nextId,
                std::vector<size_t> &knownIds) {
  static std::uniform_int_distribution<int> kindPick(0, 1);
  static std::uniform_int_distribution<size_t> pricePick(950, 1050);
  static std::uniform_int_distribution<size_t> quantityPick(1, 20);
  static std::uniform_int_distribution<int> sidePick(0, 1);

  bool wantCancel = kindPick(rng) == 1 && !knownIds.empty();

  RandomOp op{};
  if (wantCancel) {
    std::uniform_int_distribution<size_t> idxPick(0, knownIds.size() - 1);
    size_t idx = idxPick(rng);
    op.isCancel = true;
    op.id = knownIds[idx];
    knownIds.erase(knownIds.begin() + static_cast<std::ptrdiff_t>(idx));
  } else {
    op.isCancel = false;
    op.id = nextId++;
    op.side = sidePick(rng) == 0 ? Side::Buy : Side::Sell;
    op.price = pricePick(rng);
    op.quantity = quantityPick(rng);
    knownIds.push_back(op.id);
  }
  return op;
}

} // namespace

TEST_CASE("OrderBook invariants hold across random operation sequences",
          "[property]") {
  // Fixed seeds -> reproducible: a failure here can be rerun with the same
  // seed to get the exact same operation sequence back.
  unsigned int seed = GENERATE(1u, 2u, 3u, 4u, 5u);
  std::mt19937 rng(seed);

  const std::array<char, 5> symbol = {'a', 'a', 'p', 'l', '\0'};
  OrderBook ob = OrderBook<2000, 500, 3000>(symbol, 950, 1050);
  size_t nextId = 0;
  std::vector<size_t> knownIds;

  // Shadow ledger: how much of each submitted order's quantity we believe
  // is still unaccounted-for, tracked using ONLY what OrderBook tells us
  // (trade ids/sizes it returns) — not by re-implementing matching logic.
  // Kept signed so a real over-matching bug shows up as negative instead
  // of silently wrapping around like size_t would.
  std::unordered_map<size_t, long long> remaining;

  constexpr int N = 200;
  for (int step = 0; step < N; ++step) {
    RandomOp op = nextOp(rng, nextId, knownIds);
    INFO("seed=" << seed << " step=" << step << " id=" << op.id
                 << (op.isCancel ? " cancel" : " add"));

    if (op.isCancel) {
      ob.cancelOrder(op.id);
      remaining.erase(op.id);
    } else {
      remaining[op.id] = static_cast<long long>(op.quantity);
      Order o(op.id, op.price, op.quantity, op.side);
      auto [ok, trades] = ob.addOrder(std::move(o));
      REQUIRE(ok);

      for (auto &t : trades) {
        // Invariant: a trade must never reference an id we didn't submit.
        REQUIRE(remaining.contains(t.buy_side_id));
        REQUIRE(remaining.contains(t.sell_side_id));

        remaining[t.buy_side_id] -= static_cast<long long>(t.size);
        remaining[t.sell_side_id] -= static_cast<long long>(t.size);

        // Invariant: no order can be matched for more than it had left.
        CHECK(remaining[t.buy_side_id] >= 0);
        CHECK(remaining[t.sell_side_id] >= 0);
      }
    }

    // Invariant: the book must never be crossed.
    auto bid = ob.bestBid();
    auto ask = ob.bestAsk();
    if (bid.has_value() && ask.has_value()) {
      CHECK(bid.value() < ask.value());
    }
  }
}
