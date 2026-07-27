#pragma once

#include "order.hpp"
#include "trade.hpp"
#include <deque>
#include <vector>

struct PriceLevel {
public:
  explicit PriceLevel(size_t level) : level_(level) {}

  size_t getLevel() const { return level_; }
  bool empty() const { return orders_.empty(); }

  bool addOrder(Order o);
  std::vector<Trade> matchOrder(Order &o);
  void cancelOrder(size_t id);

private:
  size_t level_;
  std::deque<Order> orders_;
};
