#include "order.hpp"
#include <../src/order_book.hpp>
#include <array>
#include <benchmark/benchmark.h>

static const std::array<char, 5> s = {'a', 'a', 'p', 'l', '\0'};

static OrderBook<5000, 1, 3000> initBook(int orders, size_t &nextId) {
  OrderBook ob = OrderBook<5000, 1, 3000>(s, 50, 2000);

  size_t buyPrice = 1000;
  size_t sellPrice = 1100;

  for (int i = 0; i < orders; ++i) {
    Order buyOrder(nextId, --buyPrice, 100, Side::Buy);
    Order sellOrder(--nextId, ++sellPrice, 100, Side::Sell);
  }

  return ob;
}

static void BM_AddRestingOrder(benchmark::State &state) {
  const int bookSize = static_cast<int>(state.range(0));
  size_t nextId = 0;

  OrderBook ob = initBook(bookSize, nextId);
  for (auto _ : state) {
    ob.addOrder(Order(++nextId, 500, 10, Side::Buy));
  }
}
BENCHMARK(BM_AddRestingOrder)->Arg(10)->Arg(100)->Arg(100000);

static void BM_MatchOrder(benchmark::State &state) {
  const int bookSize = static_cast<int>(state.range(0));
  size_t nextId = 0;
  size_t trades = 0;

  OrderBook ob = initBook(bookSize, nextId);

  for (auto _ : state) {
    state.PauseTiming();
    ob.addOrder(Order(++nextId, 1050, 1, Side::Sell));
    ob.addOrder(Order(++nextId, 1055, 1, Side::Sell));
    ob.addOrder(Order(++nextId, 1056, 1, Side::Sell));

    ob.addOrder(Order(++nextId, 1058, 10, Side::Sell));
    state.ResumeTiming();

    auto [ok, t1] = ob.addOrder(Order(++nextId, 1057, 3, Side::Buy));
    auto [ok2, t2] = ob.addOrder(Order(++nextId, 1058, 10, Side::Buy));

    state.PauseTiming();
    trades += t1.size();
    trades += t2.size();
    state.ResumeTiming();
  }

  state.counters["trades"] = static_cast<double>(trades);
}
BENCHMARK(BM_MatchOrder)->Arg(10)->Arg(100)->Arg(100000);

static void BM_CancelOrder(benchmark::State &state) {
  const int bookSize = static_cast<int>(state.range(0));
  size_t nextId = 0;

  OrderBook ob = initBook(bookSize, nextId);

  for (auto _ : state) {
    state.PauseTiming();
    ob.addOrder(Order(++nextId, 1050, 1, Side::Sell));
    state.ResumeTiming();

    ob.cancelOrder(nextId);
  }
}
BENCHMARK(BM_CancelOrder)->Arg(10)->Arg(100)->Arg(100000);
