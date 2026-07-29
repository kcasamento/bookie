#pragma once

#include "order.hpp"
#include "trade.hpp"
#include <vector>

struct PriceLevel {
public:
  explicit PriceLevel(size_t level) : level_(level) {
    head_ = nullptr;
    tail_ = nullptr;
  }

  size_t getLevel() const { return level_; }
  bool empty() const { return head_ == nullptr; }

  bool addOrder(Order *o);
  std::vector<Trade> matchOrder(Order &o);
  void cancelOrder(Order *o);

private:
  Order *head_;
  Order *tail_;
  size_t level_;

  void append(Order *o);
  Order *pop();
  Order *dequeue();
  void remove(Order *o);
};
