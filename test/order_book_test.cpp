#include "order.hpp"
#include "order_book.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

const std::array<char, 5> symbol = {'a', 'a', 'p', 'p', '\0'};

TEST_CASE("add order, out of band gets rejected", "[add_order]") {
  OrderBook ob = OrderBook<100, 100, 5000>(symbol, 500, 2000);

  Order obuy = Order(1, 1000, 10, Side::Buy);
  Order osell = Order(2, 400, 10, Side::Sell);

  {
    auto [ok, trades] = ob.addOrder(std::move(osell));
    REQUIRE(!ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  auto ask = ob.bestAsk();
  REQUIRE(!ask.has_value());

  auto bid = ob.bestBid();
  REQUIRE(bid.has_value());
  REQUIRE(bid.value() == obuy.price);
}

TEST_CASE("add order, rests", "[add_order]") {
  OrderBook ob = OrderBook<100, 100, 5000>(symbol, 500, 2000);

  Order obuy = Order(1, 1000, 10, Side::Buy);
  Order osell = Order(2, 1100, 10, Side::Sell);

  {
    auto [ok, trades] = ob.addOrder(std::move(osell));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  auto ask = ob.bestAsk();
  REQUIRE(ask.has_value());
  REQUIRE(ask.value() == osell.price);

  auto bid = ob.bestBid();
  REQUIRE(bid.has_value());
  REQUIRE(bid.value() == obuy.price);
}

TEST_CASE("add order, fully filled", "[add_order]") {
  OrderBook ob = OrderBook<100, 100, 5000>(symbol, 500, 2000);

  Order obuy(1, 1000, 10, Side::Buy);
  Order osell(2, 900, 10, Side::Sell);

  {
    auto [ok, trades] = ob.addOrder(std::move(osell));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy));
    REQUIRE(ok);
    REQUIRE(trades.size() == 1);
    REQUIRE(trades.at(0).trade_price == osell.price);
  }

  auto ask = ob.bestAsk();
  REQUIRE(!ask.has_value());

  auto bid = ob.bestBid();
  REQUIRE(!bid.has_value());
}

TEST_CASE("add order, fully filled, multiple trades", "[add_order]") {
  OrderBook ob = OrderBook<100, 100, 5000>(symbol, 500, 2000);

  Order obuy(1, 1000, 3, Side::Buy);
  Order osell1(2, 800, 1, Side::Sell);
  Order osell2(3, 900, 1, Side::Sell);
  Order osell3(4, 1000, 1, Side::Sell);

  {
    auto [ok, trades] = ob.addOrder(std::move(osell1));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(osell2));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(osell3));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy));
    REQUIRE(ok);
    REQUIRE(trades.size() == 3);
    REQUIRE(trades.at(0).trade_price == osell1.price);
    REQUIRE(trades.at(1).trade_price == osell2.price);
    REQUIRE(trades.at(2).trade_price == osell3.price);
  }

  auto ask = ob.bestAsk();
  REQUIRE(!ask.has_value());

  auto bid = ob.bestBid();
  REQUIRE(!bid.has_value());
}

TEST_CASE("cancel resting order", "[cancel_order]") {
  OrderBook ob = OrderBook<100, 100, 5000>(symbol, 100, 2000);

  Order obuy1(1, 100, 3, Side::Buy);
  Order obuy2(2, 400, 3, Side::Buy);
  Order osell1(4, 900, 1, Side::Sell);
  Order osell2(3, 800, 1, Side::Sell);
  Order osell3(5, 1000, 1, Side::Sell);

  {
    auto [ok, trades] = ob.addOrder(std::move(osell1));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(osell2));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(osell3));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy1));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto [ok, trades] = ob.addOrder(std::move(obuy2));
    REQUIRE(ok);
    REQUIRE(trades.size() == 0);
  }

  {
    auto ask = ob.bestAsk();
    REQUIRE(ask.has_value());
    REQUIRE(ask.value() == osell2.price);

    auto bid = ob.bestBid();
    REQUIRE(bid.has_value());
    REQUIRE(bid.value() == obuy2.price);
  }

  ob.cancelOrder(2);

  {
    auto bid = ob.bestBid();
    REQUIRE(bid.has_value());
    REQUIRE(bid.value() == obuy1.price);
  }
}
